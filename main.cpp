#include "physics/definitions/calculate.hpp"
#include "display/definitions/display.hpp"
#include "bodies.hpp"
#include "types.hpp"

#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Physics;
using namespace Display;

int main(int argc, char* argv[]) {
    int auto_play_frames = (argc >= 3 && std::string(argv[1]) == "--auto-play") ? std::atoi(argv[2]) : 0;

    SimulationToggleParams toggle_params;
    toggle_params.comparison_mode = true;
    toggle_params.use_rk4 = true;
    toggle_params.use_euler = true;
    toggle_params.use_verlet = true;

    SimulationParams simulation_params;
    simulation_params.dt = 1.0;
    simulation_params.bodies = defaultFigure8Bodies();

    try {
        DisplayController display_controller;
        display_controller.run(simulation_params, toggle_params, auto_play_frames);
    } catch (const std::exception& e) {
#ifdef _WIN32
        MessageBoxA(nullptr, e.what(), "NBodyProblem", MB_ICONERROR | MB_OK);
#endif
        return 1;
    } catch (...) {
#ifdef _WIN32
        MessageBoxA(
            nullptr,
            "The simulation failed to start. Run from the dist\\ folder "
            "(use Run NBodyProblem.bat) so SFML DLLs are found.",
            "NBodyProblem",
            MB_ICONERROR | MB_OK);
#endif
        return 1;
    }
    return 0;
}
