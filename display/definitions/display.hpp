#pragma once
#include "display/definitions/body_editor.hpp"
#include "types.hpp"
#include "display/definitions/grid.hpp"
#include "physics/definitions/calculate.hpp"
#include <SFML/Graphics.hpp>
#include <deque>
#include <string>
#include <vector>

namespace Display {

class DisplayController {
public:
    DisplayController();
    ~DisplayController();

    void run(SimulationParams& sim_params, SimulationToggleParams& toggle_params, int auto_play_frames = 0);

private:
    static constexpr int CONTRAIL_LENGTH = 45;

    sf::RenderWindow window_;
    sf::Font font_;
    GridView grid_;
    Physics::PhysicsController physics_;
    BodyEditor body_editor_;

    SimulationParams sim_params_;
    SimulationToggleParams toggle_params_;
    SimulationPayload payload_;

    bool playing_ = false;
    float seek_slider_value_ = 0.f;
    float zoom_slider_value_ = 0.5f;
    float max_seek_time_ = 100.f;
    double last_rendered_time_ = -1.0;

    struct WorldPoint {
        double x = 0.0;
        double y = 0.0;
    };

    struct ContrailStore {
        std::deque<WorldPoint> rk4;
        std::deque<WorldPoint> euler;
        std::deque<WorldPoint> verlet;
    };
    std::vector<ContrailStore> contrails_;
    std::vector<std::vector<sf::Vertex>> contrail_draw_buffers_;

    bool font_loaded_ = false;
    bool loadFont();
    void handleEvents();
    void updateSimulation();
    void updateContrails();
    void drawUi();
    void drawBodies();
    void drawContrails();
    void drawComparisonLinks();
    sf::Color integratorColor(Physics::IntegratorKind kind) const;
    float massLabelOffsetY(Physics::IntegratorKind kind) const;
    bool pointInRect(const sf::Vector2f& point, const sf::FloatRect& rect) const;
    void applyDtSelection(int index);
    void syncToggleMode();
    void refreshSimulationView();
    void applyBodyEditor();
    void loadBodyPreset(const std::vector<MassPoint>& preset);
    void resetBodiesToDefaults();
    void loadLagrangePreset();
    void loadEulerCollinearPreset();
    void resetZoomToFit();
    double scaleFromZoomSlider() const;
    float zoomSliderFromScale(double scale) const;
    sf::FloatRect zoomSliderTrack() const;
    void setZoomFromMouseX(float mouse_x);
};

}
