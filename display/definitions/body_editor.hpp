#pragma once
#include "types.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

namespace Display {

class BodyEditor {
public:
    static constexpr int BODY_COUNT = 3;
    static constexpr int FIELDS_PER_BODY = 5;

    BodyEditor();

    enum class FieldKind { Mass, X, Y, Vx, Vy };

    struct Field {
        int body_index = 0;
        FieldKind kind = FieldKind::Mass;
        std::string label;
        std::string text;
        sf::FloatRect rect{};
    };

    void syncFromBodies(const std::vector<MassPoint>& bodies);
    bool applyToBodies(std::vector<MassPoint>& bodies) const;
    void loadDefaults(const std::vector<MassPoint>& defaults);

    bool handleMousePress(const sf::Vector2f& mouse);
    void handleTextEntered(sf::Uint32 unicode);
    void handleKeyPressed(sf::Keyboard::Key key);
    void clearFocus();

    bool contains(const sf::Vector2f& point) const;
    sf::FloatRect applyButtonRect() const { return apply_btn_; }
    sf::FloatRect figure8ButtonRect() const { return figure8_btn_; }
    sf::FloatRect lagrangeButtonRect() const { return lagrange_btn_; }
    sf::FloatRect collinearButtonRect() const { return collinear_btn_; }

    void draw(sf::RenderTarget& target, const sf::Font& font) const;

    float panelLeft() const { return panel_left_; }
    float panelWidth() const { return panel_width_; }

private:
    float panel_left_ = 920.f;
    float panel_width_ = 360.f;
    sf::FloatRect apply_btn_{};
    sf::FloatRect figure8_btn_{};
    sf::FloatRect lagrange_btn_{};
    sf::FloatRect collinear_btn_{};
    std::vector<Field> fields_;
    int focused_index_ = -1;

    static std::string formatValue(FieldKind kind, double value);
    static bool parseValue(FieldKind kind, const std::string& text, double& out);
    void layoutButtons();
    void layoutFields();
};

}
