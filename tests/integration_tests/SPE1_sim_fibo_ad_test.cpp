/*
  Copyright 2013 Statoil ASA

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE SimFiboADTest

#include <boost/test/unit_test.hpp>
#include <opm/core/pressure/FlowBCManager.hpp>

#include <opm/core/grid.h>
#include <opm/core/grid/GridManager.hpp>
#include <opm/core/wells.h>
#include <opm/core/wells/WellsManager.hpp>
#include <opm/core/utility/ErrorMacros.hpp>
#include <opm/core/simulator/initState.hpp>
#include <opm/core/simulator/SimulatorReport.hpp>
#include <opm/core/simulator/SimulatorTimer.hpp>
#include <opm/core/utility/miscUtilities.hpp>
#include <opm/core/utility/parameters/ParameterGroup.hpp>


#include <opm/core/props/BlackoilPropertiesBasic.hpp>
#include <opm/core/props/BlackoilPropertiesFromDeck.hpp>
#include <opm/core/props/rock/RockCompressibility.hpp>

#include <opm/core/linalg/LinearSolverFactory.hpp>

#include <opm/core/simulator/BlackoilState.hpp>
#include <opm/core/simulator/WellState.hpp>

#include <opm/autodiff/SimulatorFullyImplicitBlackoil.hpp>
#include <opm/autodiff/BlackoilPropsAdFromDeck.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>


#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>

#include <algorithm>
#include <iostream>
#include <vector>
#include <numeric>


using namespace Opm;

std::vector<BlackoilState> runWithNewParser(parameter::ParameterGroup param) {
    
    ParserPtr parser(new Parser());
    DeckConstPtr deck = parser->parse("SPE1_opm.DATA");
    ScheduleConstPtr schedule_deck(new Schedule(deck));
    
    std::cout << "Heisann! \n ";
    boost::scoped_ptr<EclipseGridParser> old_deck;
    boost::scoped_ptr<GridManager> grid;
    boost::scoped_ptr<BlackoilPropertiesInterface> props;
    boost::scoped_ptr<BlackoilPropsAdInterface> new_props;
    boost::scoped_ptr<RockCompressibility> rock_comp;
    BlackoilState state;
    std::vector<BlackoilState> state_collection;

    double gravity[3] = {0.0};
    std::string deck_filename = "SPE1_opm.DATA";
    old_deck.reset(new EclipseGridParser(deck_filename));
    // Grid init
    grid.reset(new GridManager(*old_deck));
    // Rock and fluid init
    props.reset(new BlackoilPropertiesFromDeck(*old_deck, *grid->c_grid(), param));
    new_props.reset(new BlackoilPropsAdFromDeck(*old_deck, *grid->c_grid()));

    rock_comp.reset(new RockCompressibility(*old_deck));

    gravity[2] = old_deck->hasField("NOGRAV") ? 0.0 : unit::gravity;

    initBlackoilStateFromDeck(*grid->c_grid(), *props, *old_deck, gravity[2], state);

    bool use_gravity = (gravity[0] != 0.0 || gravity[1] != 0.0 || gravity[2] != 0.0);
    const double *grav = use_gravity ? &gravity[0] : 0;

    // Boundary conditions.
    FlowBCManager bcs;

    // Linear solver.
    LinearSolverFactory linsolver(param);

    std::cout << "\n\n================    Starting main simulation loop     ===============\n"
            << "                        (number of epochs: "
            << (old_deck->numberOfEpochs()) << ")\n\n" << std::flush;

    SimulatorReport rep;
    // With a deck, we may have more epochs etc.
    WellState well_state;
    int step = 0;
    SimulatorTimer simtimer;
    // Use timer for last epoch to obtain total time.
    old_deck->setCurrentEpoch(old_deck->numberOfEpochs() - 1);
    simtimer.init(*old_deck);
    const double total_time = simtimer.totalTime();
    int numEpochsOld = old_deck->numberOfEpochs();
    int numEpochsNew = schedule_deck->getTimeMap()->size();
    
    std::cout << "Old " << numEpochsOld << " New: " << numEpochsNew << "\n";

    for (size_t epoch = 0; epoch < schedule_deck->getTimeMap()->size() - 1; ++epoch) { //We also have the startdate a timestep
        // Set epoch index
        old_deck->setCurrentEpoch(epoch);

        // Update the timer.
        if (old_deck->hasField("TSTEP")) {
            simtimer.init(*old_deck);
        } else {
            if (epoch != 0) {
                OPM_THROW(std::runtime_error, "No TSTEP in deck for epoch " << epoch);
            }
            simtimer.init(param);
        }
        simtimer.setCurrentStepNum(step);
        simtimer.setTotalTime(total_time);

        // Report on start of epoch.
        std::cout << "\n\n--------------    Starting epoch " << epoch << "    --------------"
                << "\n                  (number of steps: "
                << simtimer.numSteps() - step << ")\n\n" << std::flush;

        // Create new wells, well_state
        WellsManager wells(*old_deck, *grid->c_grid(), props->permeability());
        // @@@ HACK: we should really make a new well state and
        // properly transfer old well state to it every epoch,
        // since number of wells may change etc.
        if (epoch == 0) {
            well_state.init(wells.c_wells(), state);
        }

        // Create and run simulator.
        SimulatorFullyImplicitBlackoil simulator(param,
                *grid->c_grid(),
                *new_props,
                rock_comp->isActive() ? rock_comp.get() : 0,
                wells,
                bcs.c_bcs(),
                linsolver,
                grav);
     
        SimulatorReport epoch_rep = simulator.run(simtimer, state, well_state);
        
        BlackoilState copy = state;
        state_collection.push_back(copy);
        // Update total timing report and remember step number.
        rep += epoch_rep;
        step = simtimer.currentStepNum();
    }

    std::cout << "\n\n================    End of simulation     ===============\n\n"; 
    rep.report(std::cout);
    
    return state_collection;
}


std::vector<BlackoilState> runWithOldParser(parameter::ParameterGroup param) {
    boost::scoped_ptr<EclipseGridParser> deck;
    boost::scoped_ptr<GridManager> grid;
    boost::scoped_ptr<BlackoilPropertiesInterface> props;
    boost::scoped_ptr<BlackoilPropsAdInterface> new_props;
    boost::scoped_ptr<RockCompressibility> rock_comp;
    BlackoilState state;
    std::vector<BlackoilState> state_collection;

    double gravity[3] = {0.0};
    std::string deck_filename = "SPE1_opm.DATA";
    deck.reset(new EclipseGridParser(deck_filename));
    // Grid init
    grid.reset(new GridManager(*deck));
    // Rock and fluid init
    props.reset(new BlackoilPropertiesFromDeck(*deck, *grid->c_grid(), param));
    new_props.reset(new BlackoilPropsAdFromDeck(*deck, *grid->c_grid()));

    rock_comp.reset(new RockCompressibility(*deck));

    gravity[2] = deck->hasField("NOGRAV") ? 0.0 : unit::gravity;

    initBlackoilStateFromDeck(*grid->c_grid(), *props, *deck, gravity[2], state);

    bool use_gravity = (gravity[0] != 0.0 || gravity[1] != 0.0 || gravity[2] != 0.0);
    const double *grav = use_gravity ? &gravity[0] : 0;

    // Boundary conditions.
    FlowBCManager bcs;

    // Linear solver.
    LinearSolverFactory linsolver(param);

    std::cout << "\n\n================    Starting main simulation loop     ===============\n"
            << "                        (number of epochs: "
            << (deck->numberOfEpochs()) << ")\n\n" << std::flush;

    SimulatorReport rep;
    // With a deck, we may have more epochs etc.
    WellState well_state;
    int step = 0;
    SimulatorTimer simtimer;
    // Use timer for last epoch to obtain total time.
    deck->setCurrentEpoch(deck->numberOfEpochs() - 1);
    simtimer.init(*deck);
    const double total_time = simtimer.totalTime();
    for (int epoch = 0; epoch < deck->numberOfEpochs(); ++epoch) {
        // Set epoch index.
        deck->setCurrentEpoch(epoch);

        // Update the timer.
        if (deck->hasField("TSTEP")) {
            simtimer.init(*deck);
        } else {
            if (epoch != 0) {
                OPM_THROW(std::runtime_error, "No TSTEP in deck for epoch " << epoch);
            }
            simtimer.init(param);
        }
        simtimer.setCurrentStepNum(step);
        simtimer.setTotalTime(total_time);

        // Report on start of epoch.
        std::cout << "\n\n--------------    Starting epoch " << epoch << "    --------------"
                << "\n                  (number of steps: "
                << simtimer.numSteps() - step << ")\n\n" << std::flush;

        // Create new wells, well_state
        WellsManager wells(*deck, *grid->c_grid(), props->permeability());
        // @@@ HACK: we should really make a new well state and
        // properly transfer old well state to it every epoch,
        // since number of wells may change etc.
        if (epoch == 0) {
            well_state.init(wells.c_wells(), state);
        }

        // Create and run simulator.
        SimulatorFullyImplicitBlackoil simulator(param,
                *grid->c_grid(),
                *new_props,
                rock_comp->isActive() ? rock_comp.get() : 0,
                wells,
                bcs.c_bcs(),
                linsolver,
                grav);
     
        SimulatorReport epoch_rep = simulator.run(simtimer, state, well_state);
        
        BlackoilState copy = state;
        state_collection.push_back(copy);
        // Update total timing report and remember step number.
        rep += epoch_rep;
        step = simtimer.currentStepNum();
    }

    std::cout << "\n\n================    End of simulation     ===============\n\n"; 
    rep.report(std::cout);
    
    return state_collection;
}



BOOST_AUTO_TEST_CASE(SPE1_runWithOldAndNewParser_BlackOilStateEqual) {
    Parser * parser = new Parser();
    DeckConstPtr deck = parser->parse("SPE1_opm.DATA", true);
    ScheduleConstPtr schedule_deck(new Schedule(deck));
    int argc = 2;
    const char ** argv = (const char **)malloc(2* sizeof(argv));
    argv[1] = "spe1.xml";
    parameter::ParameterGroup param(argc, argv, false);
    
    std::vector<BlackoilState> runWithOldParserStates = runWithOldParser(param);
    std::vector<BlackoilState> runWithNewParserStates = runWithNewParser(param);
    
    for(size_t i=0; i<runWithOldParserStates.size(); i++) {
        BOOST_CHECK(runWithOldParserStates[i].equals(runWithNewParserStates[i]));
    }
}

