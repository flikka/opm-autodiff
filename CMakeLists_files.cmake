# -*- mode: cmake; tab-width: 2; indent-tabs-mode: t; truncate-lines: t; compile-command: "cmake -Wdev" -*-
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 noexpandtab softtabstop=2 nowrap:

# This file sets up five lists:
#	MAIN_SOURCE_FILES     List of compilation units which will be included in
#	                      the library. If it isn't on this list, it won't be
#	                      part of the library. Please try to keep it sorted to
#	                      maintain sanity.
#
#	TEST_SOURCE_FILES     List of programs that will be run as unit tests.
#
#	TEST_DATA_FILES       Files from the source three that should be made
#	                      available in the corresponding location in the build
#	                      tree in order to run tests there.
#
#	EXAMPLE_SOURCE_FILES  Other programs that will be compiled as part of the
#	                      build, but which is not part of the library nor is
#	                      run as tests.
#
#	PUBLIC_HEADER_FILES   List of public header files that should be
#	                      distributed together with the library. The source
#	                      files can of course include other files than these;
#	                      you should only add to this list if the *user* of
#	                      the library needs it.

# originally generated with the command:
# find opm -name '*.c*' -printf '\t%p\n' | sort
list (APPEND MAIN_SOURCE_FILES
	opm/autodiff/BlackoilPropsAd.cpp
	opm/autodiff/BlackoilPropsAdInterface.cpp
	opm/autodiff/NewtonIterationBlackoilCPR.cpp
	opm/autodiff/NewtonIterationBlackoilSimple.cpp
	opm/autodiff/GridHelpers.cpp
	opm/autodiff/ImpesTPFAAD.cpp
	opm/autodiff/SimulatorCompressibleAd.cpp
	opm/autodiff/SimulatorFullyImplicitBlackoilOutput.cpp
	opm/autodiff/SimulatorIncompTwophaseAd.cpp
	opm/autodiff/TransportSolverTwophaseAd.cpp
	opm/autodiff/BlackoilPropsAdFromDeck.cpp
	opm/autodiff/WellDensitySegmented.cpp
	opm/autodiff/LinearisedBlackoilResidual.cpp
	)

# originally generated with the command:
# find tests -name '*.cpp' -a ! -wholename '*/not-unit/*' -printf '\t%p\n' | sort
list (APPEND TEST_SOURCE_FILES
	tests/test_block.cpp
	tests/test_boprops_ad.cpp
	tests/test_span.cpp
	tests/test_syntax.cpp
	tests/test_scalar_mult.cpp
	tests/test_welldensitysegmented.cpp
	)

list (APPEND TEST_DATA_FILES
	tests/fluid.data
	)

# Note, these two files are not included in the repo.
# If enabling INCLUDE_NON_PUBLIC_TESTS, please add add symlink to the folder
# "non_public" containing these two files.
if (INCLUDE_NON_PUBLIC_TESTS)
	list (APPEND TEST_DATA_FILES
		tests/non_public/SPE1_opm.DATA
		tests/non_public/spe1.xml
		)
endif()


# originally generated with the command:
# find tutorials examples -name '*.c*' -printf '\t%p\n' | sort
list (APPEND EXAMPLE_SOURCE_FILES
	examples/find_zero.cpp
	examples/sim_fibo_ad.cpp
	examples/sim_2p_comp_ad.cpp
	examples/sim_2p_incomp_ad.cpp
	examples/sim_simple.cpp
	examples/test_impestpfa_ad.cpp
	examples/test_implicit_ad.cpp
	)

# programs listed here will not only be compiled, but also marked for
# installation
list (APPEND PROGRAM_SOURCE_FILES
	examples/sim_2p_incomp_ad.cpp
	examples/sim_fibo_ad.cpp
	)

# originally generated with the command:
# find opm -name '*.h*' -a ! -name '*-pch.hpp' -printf '\t%p\n' | sort
list (APPEND PUBLIC_HEADER_FILES
	opm/autodiff/AutoDiffBlock.hpp
	opm/autodiff/AutoDiffHelpers.hpp
	opm/autodiff/AutoDiff.hpp
	opm/autodiff/BlackoilPropsAd.hpp
	opm/autodiff/BlackoilPropsAdFromDeck.hpp
	opm/autodiff/BlackoilPropsAdInterface.hpp
	opm/autodiff/CPRPreconditioner.hpp
	opm/autodiff/GeoProps.hpp
	opm/autodiff/GridHelpers.hpp
	opm/autodiff/ImpesTPFAAD.hpp
	opm/autodiff/FullyImplicitBlackoilSolver.hpp
	opm/autodiff/FullyImplicitBlackoilSolver_impl.hpp
	opm/autodiff/NewtonIterationBlackoilCPR.hpp
	opm/autodiff/NewtonIterationBlackoilInterface.hpp
	opm/autodiff/NewtonIterationBlackoilSimple.hpp
	opm/autodiff/LinearisedBlackoilResidual.hpp
	opm/autodiff/SimulatorCompressibleAd.hpp
	opm/autodiff/SimulatorFullyImplicitBlackoil.hpp
	opm/autodiff/SimulatorFullyImplicitBlackoil_impl.hpp
	opm/autodiff/SimulatorIncompTwophaseAd.hpp
	opm/autodiff/TransportSolverTwophaseAd.hpp
	opm/autodiff/WellDensitySegmented.hpp
	opm/autodiff/WellStateFullyImplicitBlackoil.hpp
	)
