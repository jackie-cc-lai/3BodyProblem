#include "display/definitions/display.hpp"
#include "bodies.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace Display {

namespace {

constexpr unsigned WINDOW_WIDTH = 1280;
constexpr unsigned WINDOW_HEIGHT = 800;
constexpr float SIM_TOP = 0.f;
constexpr float SIM_HEIGHT = 620.f;
constexpr float UI_TOP = 620.f;
constexpr float UI_HEIGHT = 180.f;

const double DT_OPTIONS[] = {0.5, 1.0, 2.0, 5.0};
const char* DT_LABELS[] = {"t=0.5", "t=1", "t=2", "t=5"};
constexpr int DT_COUNT = 4;

constexpr float SETUP_PANEL_WIDTH = 360.f;
constexpr float SIM_VIEW_WIDTH = WINDOW_WIDTH - SETUP_PANEL_WIDTH;

sf::FloatRect makeRect(float x, float y, float w, float h) {
    return {x, y, w, h};
}

} // namespace

DisplayController::DisplayController()
    : window_(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "3-Body Problem Simulation")
{
    window_.setFramerateLimit(60);
    loadFont();
}

DisplayController::~DisplayController() = default;

bool DisplayController::loadFont() {
    const char* candidates[] = {
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf",
    };
    for (const char* path : candidates) {
        if (font_.loadFromFile(path)) {
            font_loaded_ = true;
            return true;
        }
    }
    font_loaded_ = false;
    return false;
}

bool DisplayController::pointInRect(const sf::Vector2f& point, const sf::FloatRect& rect) const {
    return rect.contains(point);
}

sf::Color DisplayController::integratorColor(Physics::IntegratorKind kind) const {
    switch (kind) {
        case Physics::IntegratorKind::Euler: return sf::Color::White;
        case Physics::IntegratorKind::RK4: return sf::Color::Red;
        case Physics::IntegratorKind::Verlet: return sf::Color::Green;
    }
    return sf::Color::White;
}

float DisplayController::massLabelOffsetY(Physics::IntegratorKind kind) const {
    if (!toggle_params_.comparison_mode) {
        return 11.f;
    }
    switch (kind) {
        case Physics::IntegratorKind::Euler: return -18.f;
        case Physics::IntegratorKind::RK4: return 11.f;
        case Physics::IntegratorKind::Verlet: return 24.f;
    }
    return 11.f;
}

void DisplayController::syncToggleMode() {
    if (toggle_params_.comparison_mode) {
        toggle_params_.use_rk4 = true;
        toggle_params_.use_euler = true;
        toggle_params_.use_verlet = true;
        return;
    }

    const int active = static_cast<int>(toggle_params_.use_rk4)
        + static_cast<int>(toggle_params_.use_euler)
        + static_cast<int>(toggle_params_.use_verlet);
    if (active != 1) {
        toggle_params_.use_rk4 = true;
        toggle_params_.use_euler = false;
        toggle_params_.use_verlet = false;
    }
}

void DisplayController::refreshSimulationView() {
    playing_ = false;
    for (auto& trail : contrails_) {
        trail.rk4.clear();
        trail.euler.clear();
        trail.verlet.clear();
    }
    last_rendered_time_ = -1.0;
    payload_ = physics_.compute(toggle_params_, sim_params_);
    sim_params_.elapsed_time = payload_.elapsed_time;
    seek_slider_value_ = static_cast<float>(payload_.elapsed_time);
    resetZoomToFit();
}

void DisplayController::applyBodyEditor() {
    if (!body_editor_.applyToBodies(sim_params_.bodies)) {
        return;
    }
    contrails_.assign(sim_params_.bodies.size(), ContrailStore{});
    refreshSimulationView();
}

void DisplayController::loadBodyPreset(const std::vector<MassPoint>& preset) {
    sim_params_.bodies = preset;
    body_editor_.loadDefaults(sim_params_.bodies);
    contrails_.assign(sim_params_.bodies.size(), ContrailStore{});
    refreshSimulationView();
}

void DisplayController::resetBodiesToDefaults() {
    loadBodyPreset(defaultFigure8Bodies());
}

void DisplayController::loadLagrangePreset() {
    loadBodyPreset(defaultLagrangeTriangleBodies());
}

void DisplayController::loadEulerCollinearPreset() {
    loadBodyPreset(defaultEulerCollinearBodies());
}

void DisplayController::applyDtSelection(int index) {
    if (index >= 0 && index < DT_COUNT) {
        sim_params_.dt = DT_OPTIONS[index];
    }
}

void DisplayController::resetZoomToFit() {
    const double fit_scale = GridView::computeFitScale(payload_, SIM_VIEW_WIDTH, SIM_HEIGHT);
    zoom_slider_value_ = zoomSliderFromScale(fit_scale);
    grid_.setScale(fit_scale);
}

double DisplayController::scaleFromZoomSlider() const {
    const double log_min = std::log(GridView::MIN_SCALE);
    const double log_max = std::log(GridView::MAX_SCALE);
    const double log_scale = log_min + static_cast<double>(zoom_slider_value_) * (log_max - log_min);
    return std::exp(log_scale);
}

float DisplayController::zoomSliderFromScale(double scale) const {
    const double log_min = std::log(GridView::MIN_SCALE);
    const double log_max = std::log(GridView::MAX_SCALE);
    const double clamped = std::max(GridView::MIN_SCALE, std::min(GridView::MAX_SCALE, scale));
    const double log_scale = std::log(clamped);
    return static_cast<float>((log_scale - log_min) / (log_max - log_min));
}

sf::FloatRect DisplayController::zoomSliderTrack() const {
    return makeRect(420.f, UI_TOP + 114.f, 300.f, 8.f);
}

void DisplayController::setZoomFromMouseX(float mouse_x) {
    const sf::FloatRect track = zoomSliderTrack();
    const float t = (mouse_x - track.left) / track.width;
    zoom_slider_value_ = std::max(0.f, std::min(1.f, t));
    grid_.setScale(scaleFromZoomSlider());
}

void DisplayController::run(SimulationParams& sim_params, SimulationToggleParams& toggle_params, int auto_play_frames) {
    if (!window_.isOpen()) {
        return;
    }

    sim_params_ = sim_params;
    toggle_params_ = toggle_params;
    contrails_.assign(sim_params_.bodies.size(), ContrailStore{});
    body_editor_.syncFromBodies(sim_params_.bodies);

    payload_ = physics_.compute(toggle_params_, sim_params_);
    sim_params.elapsed_time = payload_.elapsed_time;
    resetZoomToFit();

    if (auto_play_frames > 0) {
        playing_ = true;
    }

    int frames_remaining = auto_play_frames;
    while (window_.isOpen()) {
        handleEvents();
        updateSimulation();

        grid_.updateViewport(SIM_VIEW_WIDTH, SIM_HEIGHT);
        grid_.setScale(scaleFromZoomSlider());
        updateContrails();

        window_.clear(sf::Color(20, 20, 24));
        grid_.draw(window_);
        drawContrails();
        drawComparisonLinks();
        drawBodies();
        if (font_loaded_) {
            body_editor_.draw(window_, font_);
        }
        drawUi();
        window_.display();

        if (frames_remaining > 0 && --frames_remaining == 0) {
            window_.close();
        }
    }

    sim_params = sim_params_;
    toggle_params = toggle_params_;
}

void DisplayController::handleEvents() {
    sf::Event event{};
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window_.close();
        }

        if (event.type == sf::Event::TextEntered) {
            body_editor_.handleTextEntered(event.text.unicode);
        }

        if (event.type == sf::Event::KeyPressed) {
            body_editor_.handleKeyPressed(event.key.code);
            if (event.key.code == sf::Keyboard::Enter) {
                applyBodyEditor();
            }
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            const sf::Vector2f mouse(static_cast<float>(event.mouseButton.x),
                                     static_cast<float>(event.mouseButton.y));

            if (body_editor_.applyButtonRect().contains(mouse)) {
                applyBodyEditor();
                continue;
            }
            if (body_editor_.figure8ButtonRect().contains(mouse)) {
                resetBodiesToDefaults();
                continue;
            }
            if (body_editor_.lagrangeButtonRect().contains(mouse)) {
                loadLagrangePreset();
                continue;
            }
            if (body_editor_.collinearButtonRect().contains(mouse)) {
                loadEulerCollinearPreset();
                continue;
            }
            if (!body_editor_.contains(mouse)) {
                body_editor_.clearFocus();
            }
            if (body_editor_.handleMousePress(mouse)) {
                continue;
            }

            const sf::FloatRect play_btn = makeRect(20.f, UI_TOP + 20.f, 80.f, 32.f);
            const sf::FloatRect pause_btn = makeRect(110.f, UI_TOP + 20.f, 80.f, 32.f);
            const sf::FloatRect reset_btn = makeRect(200.f, UI_TOP + 20.f, 80.f, 32.f);
            const sf::FloatRect seek_btn = makeRect(290.f, UI_TOP + 20.f, 80.f, 32.f);
            const sf::FloatRect compare_btn = makeRect(400.f, UI_TOP + 20.f, 110.f, 32.f);
            const sf::FloatRect rk4_btn = makeRect(518.f, UI_TOP + 20.f, 72.f, 32.f);
            const sf::FloatRect euler_btn = makeRect(596.f, UI_TOP + 20.f, 72.f, 32.f);
            const sf::FloatRect verlet_btn = makeRect(674.f, UI_TOP + 20.f, 72.f, 32.f);

            if (pointInRect(mouse, play_btn)) {
                playing_ = true;
            } else if (pointInRect(mouse, pause_btn)) {
                playing_ = false;
            } else if (pointInRect(mouse, reset_btn)) {
                playing_ = false;
                sim_params_.reset_requested = true;
                seek_slider_value_ = 0.f;
                for (auto& trail : contrails_) {
                    trail.rk4.clear();
                    trail.euler.clear();
                    trail.verlet.clear();
                }
                last_rendered_time_ = -1.0;
                payload_ = physics_.compute(toggle_params_, sim_params_);
                sim_params_.elapsed_time = payload_.elapsed_time;
                resetZoomToFit();
            } else if (pointInRect(mouse, seek_btn)) {
                playing_ = false;
                sim_params_.seek_requested = true;
                sim_params_.seek_target_time = seek_slider_value_;
                for (auto& trail : contrails_) {
                    trail.rk4.clear();
                    trail.euler.clear();
                    trail.verlet.clear();
                }
                last_rendered_time_ = -1.0;
                payload_ = physics_.compute(toggle_params_, sim_params_);
                sim_params_.elapsed_time = payload_.elapsed_time;
            } else if (pointInRect(mouse, compare_btn)) {
                toggle_params_.comparison_mode = !toggle_params_.comparison_mode;
                syncToggleMode();
                refreshSimulationView();
            } else if (pointInRect(mouse, rk4_btn)) {
                toggle_params_.comparison_mode = false;
                toggle_params_.use_rk4 = true;
                toggle_params_.use_euler = false;
                toggle_params_.use_verlet = false;
                refreshSimulationView();
            } else if (pointInRect(mouse, euler_btn)) {
                toggle_params_.comparison_mode = false;
                toggle_params_.use_rk4 = false;
                toggle_params_.use_euler = true;
                toggle_params_.use_verlet = false;
                refreshSimulationView();
            } else if (pointInRect(mouse, verlet_btn)) {
                toggle_params_.comparison_mode = false;
                toggle_params_.use_rk4 = false;
                toggle_params_.use_euler = false;
                toggle_params_.use_verlet = true;
                refreshSimulationView();
            }

            for (int i = 0; i < DT_COUNT; ++i) {
                const sf::FloatRect dt_btn = makeRect(20.f + i * 90.f, UI_TOP + 70.f, 80.f, 28.f);
                if (pointInRect(mouse, dt_btn)) {
                    applyDtSelection(i);
                }
            }

            const sf::FloatRect zoom_track = zoomSliderTrack();
            if (pointInRect(mouse, zoom_track)) {
                setZoomFromMouseX(mouse.x);
            }
        }

        if (event.type == sf::Event::MouseMoved) {
            const sf::FloatRect seek_track = makeRect(420.f, UI_TOP + 70.f, 300.f, 8.f);
            const sf::FloatRect zoom_track = zoomSliderTrack();
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                const sf::Vector2f move_mouse(static_cast<float>(event.mouseMove.x),
                                              static_cast<float>(event.mouseMove.y));
                if (seek_track.contains(move_mouse)) {
                    const float t = (move_mouse.x - seek_track.left) / seek_track.width;
                    seek_slider_value_ = std::max(0.f, std::min(1.f, t)) * static_cast<float>(max_seek_time_);
                } else if (zoom_track.contains(move_mouse)) {
                    setZoomFromMouseX(move_mouse.x);
                }
            }
        }
    }
}

void DisplayController::updateSimulation() {
    if (playing_) {
        sim_params_.step_requested = true;
        payload_ = physics_.compute(toggle_params_, sim_params_);
        sim_params_.elapsed_time = payload_.elapsed_time;
        seek_slider_value_ = static_cast<float>(payload_.elapsed_time);
        max_seek_time_ = std::max(max_seek_time_, static_cast<float>(payload_.elapsed_time + 5.0));
    }
}

void DisplayController::updateContrails() {
    if (payload_.elapsed_time == last_rendered_time_) {
        return;
    }
    last_rendered_time_ = payload_.elapsed_time;

    const std::size_t body_count = std::max({payload_.rk4_data.size(),
        payload_.euler_data.size(), payload_.verlet_data.size()});
    if (contrails_.size() < body_count) {
        contrails_.assign(body_count, ContrailStore{});
    }

    for (std::size_t i = 0; i < payload_.rk4_data.size(); ++i) {
        auto& trail = contrails_[i].rk4;
        trail.push_back({payload_.rk4_data[i].x, payload_.rk4_data[i].y});
        while (trail.size() > CONTRAIL_LENGTH) {
            trail.pop_front();
        }
    }

    for (std::size_t i = 0; i < payload_.euler_data.size(); ++i) {
        auto& trail = contrails_[i].euler;
        trail.push_back({payload_.euler_data[i].x, payload_.euler_data[i].y});
        while (trail.size() > CONTRAIL_LENGTH) {
            trail.pop_front();
        }
    }

    for (std::size_t i = 0; i < payload_.verlet_data.size(); ++i) {
        auto& trail = contrails_[i].verlet;
        trail.push_back({payload_.verlet_data[i].x, payload_.verlet_data[i].y});
        while (trail.size() > CONTRAIL_LENGTH) {
            trail.pop_front();
        }
    }
}

void DisplayController::drawContrails() {
    contrail_draw_buffers_.clear();

    auto draw_path = [&](const std::deque<WorldPoint>& trail, const sf::Color& color) {
        if (trail.size() < 2) {
            return;
        }

        contrail_draw_buffers_.emplace_back();
        auto& vertices = contrail_draw_buffers_.back();
        vertices.reserve(trail.size());
        for (const auto& point : trail) {
            vertices.push_back({grid_.worldToScreen(point.x, point.y), color});
        }
        window_.draw(vertices.data(), vertices.size(), sf::LineStrip);
    };

    for (std::size_t i = 0; i < contrails_.size(); ++i) {
        if (!payload_.rk4_data.empty()) {
            draw_path(contrails_[i].rk4, integratorColor(Physics::IntegratorKind::RK4));
        }
        if (!payload_.euler_data.empty()) {
            draw_path(contrails_[i].euler, integratorColor(Physics::IntegratorKind::Euler));
        }
        if (!payload_.verlet_data.empty()) {
            draw_path(contrails_[i].verlet, integratorColor(Physics::IntegratorKind::Verlet));
        }
    }
}

void DisplayController::drawComparisonLinks() {
    if (!toggle_params_.comparison_mode) {
        return;
    }

    const std::size_t n = payload_.rk4_data.size();
    if (n == 0 || n != payload_.euler_data.size() || n != payload_.verlet_data.size()) {
        return;
    }

    const sf::Color link_color(180, 180, 180, 80);
    for (std::size_t i = 0; i < n; ++i) {
        const sf::Vector2f positions[] = {
            grid_.worldToScreen(payload_.rk4_data[i].x, payload_.rk4_data[i].y),
            grid_.worldToScreen(payload_.euler_data[i].x, payload_.euler_data[i].y),
            grid_.worldToScreen(payload_.verlet_data[i].x, payload_.verlet_data[i].y),
        };
        for (int a = 0; a < 3; ++a) {
            for (int b = a + 1; b < 3; ++b) {
                sf::Vertex line[] = {
                    {positions[a], link_color},
                    {positions[b], link_color}
                };
                window_.draw(line, 2, sf::Lines);
            }
        }
    }
}

void DisplayController::drawBodies() {
    auto draw_set = [&](const std::vector<MassPoint>& bodies, Physics::IntegratorKind kind) {
        const sf::Color color = integratorColor(kind);
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            const auto pos = grid_.worldToScreen(bodies[i].x, bodies[i].y);
            sf::CircleShape body(7.f);
            body.setOrigin(7.f, 7.f);
            body.setPosition(pos);
            body.setFillColor(color);
            window_.draw(body);

            if (!font_loaded_) {
                continue;
            }

            const std::string label = "m" + std::to_string(i + 1);
            sf::Text tag(label, font_, 11);
            tag.setFillColor(color);
            const auto bounds = tag.getLocalBounds();
            tag.setPosition(pos.x - bounds.width / 2.f - bounds.left,
                            pos.y + massLabelOffsetY(kind) - bounds.top);
            window_.draw(tag);
        }
    };

    if (!payload_.rk4_data.empty()) {
        draw_set(payload_.rk4_data, Physics::IntegratorKind::RK4);
    }
    if (!payload_.euler_data.empty()) {
        draw_set(payload_.euler_data, Physics::IntegratorKind::Euler);
    }
    if (!payload_.verlet_data.empty()) {
        draw_set(payload_.verlet_data, Physics::IntegratorKind::Verlet);
    }
}

void DisplayController::drawUi() {
    sf::RectangleShape panel({WINDOW_WIDTH, UI_HEIGHT});
    panel.setPosition(0.f, UI_TOP);
    panel.setFillColor(sf::Color(30, 30, 36));
    window_.draw(panel);

    if (!font_loaded_) {
        return;
    }

    auto draw_button = [&](const sf::FloatRect& rect, const std::string& label, bool active) {
        sf::RectangleShape btn({rect.width, rect.height});
        btn.setPosition(rect.left, rect.top);
        btn.setFillColor(active ? sf::Color(70, 110, 170) : sf::Color(55, 55, 65));
        btn.setOutlineColor(sf::Color(90, 90, 100));
        btn.setOutlineThickness(1.f);
        window_.draw(btn);

        sf::Text text(label, font_, 14);
        text.setFillColor(sf::Color::White);
        const auto bounds = text.getLocalBounds();
        text.setPosition(rect.left + (rect.width - bounds.width) / 2.f - bounds.left,
                         rect.top + (rect.height - bounds.height) / 2.f - bounds.top);
        window_.draw(text);
    };

    draw_button(makeRect(20.f, UI_TOP + 20.f, 80.f, 32.f), "Play", playing_);
    draw_button(makeRect(110.f, UI_TOP + 20.f, 80.f, 32.f), "Pause", !playing_);
    draw_button(makeRect(200.f, UI_TOP + 20.f, 80.f, 32.f), "Reset", false);
    draw_button(makeRect(290.f, UI_TOP + 20.f, 80.f, 32.f), "Seek", false);
    draw_button(makeRect(400.f, UI_TOP + 20.f, 110.f, 32.f), "Compare", toggle_params_.comparison_mode);
    draw_button(makeRect(518.f, UI_TOP + 20.f, 72.f, 32.f), "RK4",
                 !toggle_params_.comparison_mode && toggle_params_.use_rk4);
    draw_button(makeRect(596.f, UI_TOP + 20.f, 72.f, 32.f), "Euler",
                 !toggle_params_.comparison_mode && toggle_params_.use_euler);
    draw_button(makeRect(674.f, UI_TOP + 20.f, 72.f, 32.f), "Verlet",
                 !toggle_params_.comparison_mode && toggle_params_.use_verlet);

    for (int i = 0; i < DT_COUNT; ++i) {
        const bool selected = std::abs(sim_params_.dt - DT_OPTIONS[i]) < 1e-12;
        draw_button(makeRect(20.f + i * 90.f, UI_TOP + 70.f, 80.f, 28.f), DT_LABELS[i], selected);
    }

    const sf::FloatRect slider_track = makeRect(420.f, UI_TOP + 74.f, 300.f, 8.f);
    sf::RectangleShape track({slider_track.width, slider_track.height});
    track.setPosition(slider_track.left, slider_track.top);
    track.setFillColor(sf::Color(50, 50, 58));
    window_.draw(track);

    const float knob_x = slider_track.left +
        (max_seek_time_ > 0.f ? (seek_slider_value_ / max_seek_time_) * slider_track.width : 0.f);
    sf::CircleShape knob(8.f);
    knob.setOrigin(8.f, 8.f);
    knob.setPosition(knob_x, slider_track.top + slider_track.height * 0.5f);
    knob.setFillColor(sf::Color(180, 180, 200));
    window_.draw(knob);

    std::ostringstream time_text;
    time_text << std::fixed << std::setprecision(4)
              << "t = " << payload_.elapsed_time;
    sf::Text time_label(time_text.str(), font_, 16);
    time_label.setFillColor(sf::Color::White);
    time_label.setPosition(780.f, UI_TOP + 22.f);
    window_.draw(time_label);

    sf::Text seek_label("Seek slider (t)", font_, 14);
    seek_label.setFillColor(sf::Color(200, 200, 210));
    seek_label.setPosition(420.f, UI_TOP + 52.f);
    window_.draw(seek_label);

    const sf::FloatRect zoom_track = zoomSliderTrack();
    sf::RectangleShape zoom_track_shape({zoom_track.width, zoom_track.height});
    zoom_track_shape.setPosition(zoom_track.left, zoom_track.top);
    zoom_track_shape.setFillColor(sf::Color(50, 50, 58));
    window_.draw(zoom_track_shape);

    const float zoom_knob_x = zoom_track.left + zoom_slider_value_ * zoom_track.width;
    sf::CircleShape zoom_knob(8.f);
    zoom_knob.setOrigin(8.f, 8.f);
    zoom_knob.setPosition(zoom_knob_x, zoom_track.top + zoom_track.height * 0.5f);
    zoom_knob.setFillColor(sf::Color(180, 180, 200));
    window_.draw(zoom_knob);

    sf::Text zoom_label("Zoom", font_, 14);
    zoom_label.setFillColor(sf::Color(200, 200, 210));
    zoom_label.setPosition(420.f, UI_TOP + 92.f);
    window_.draw(zoom_label);

    sf::Text units_label(
        "Masses in M_sun; positions in AU; velocities in km/s; 1 t = 4.74 yr",
        font_, 13);
    units_label.setFillColor(sf::Color(160, 160, 170));
    units_label.setPosition(780.f, UI_TOP + 52.f);
    window_.draw(units_label);
}

}
