#include "display/definitions/body_editor.hpp"
#include <iomanip>
#include <sstream>

namespace Display {

namespace {

constexpr float PANEL_TOP = 0.f;
constexpr float PANEL_HEIGHT = 620.f;
constexpr float PAD = 12.f;
constexpr float ROW_H = 22.f;
constexpr float LABEL_W = 28.f;
constexpr float FIELD_W = 108.f;
constexpr float BODY_HEADER_H = 20.f;
constexpr float BODY_BLOCK_H = 96.f;
constexpr float APPLY_ROW_Y = 36.f;
constexpr float PRESET_ROW_Y = 66.f;
constexpr float BODY_BLOCKS_TOP = 98.f;
constexpr float BTN_H = 26.f;

bool isNumericChar(sf::Uint32 unicode) {
    return (unicode >= '0' && unicode <= '9') || unicode == '.' || unicode == '-' || unicode == '+'
        || unicode == 'e' || unicode == 'E';
}

} // namespace

BodyEditor::BodyEditor() {
    layoutButtons();
}

void BodyEditor::layoutButtons() {
    panel_left_ = 920.f;
    panel_width_ = 360.f;

    const float inner_w = panel_width_ - 2.f * PAD;
    apply_btn_ = {panel_left_ + PAD, PANEL_TOP + APPLY_ROW_Y, inner_w, BTN_H};

    const float preset_gap = 4.f;
    const float preset_w = (inner_w - 2.f * preset_gap) / 3.f;
    figure8_btn_ = {panel_left_ + PAD, PANEL_TOP + PRESET_ROW_Y, preset_w, BTN_H};
    lagrange_btn_ = {figure8_btn_.left + preset_w + preset_gap, PANEL_TOP + PRESET_ROW_Y, preset_w, BTN_H};
    collinear_btn_ = {lagrange_btn_.left + preset_w + preset_gap, PANEL_TOP + PRESET_ROW_Y, preset_w, BTN_H};
}

void BodyEditor::layoutFields() {
    layoutButtons();
    fields_.clear();
    fields_.reserve(BODY_COUNT * FIELDS_PER_BODY);

    const char* labels[] = {"m", "x", "y", "vx", "vy"};
    const FieldKind kinds[] = {
        FieldKind::Mass, FieldKind::X, FieldKind::Y, FieldKind::Vx, FieldKind::Vy
    };

    for (int body = 0; body < BODY_COUNT; ++body) {
        const float block_top = PANEL_TOP + BODY_BLOCKS_TOP + body * BODY_BLOCK_H;
        for (int field = 0; field < FIELDS_PER_BODY; ++field) {
            const int row = field / 2;
            const int col = field % 2;
            const float x = panel_left_ + PAD + col * (LABEL_W + FIELD_W + 16.f);
            const float y = block_top + BODY_HEADER_H + row * (ROW_H + 6.f);

            Field entry{};
            entry.body_index = body;
            entry.kind = kinds[field];
            entry.label = labels[field];
            entry.rect = {x + LABEL_W, y, FIELD_W, ROW_H};
            fields_.push_back(std::move(entry));
        }
    }
}

std::string BodyEditor::formatValue(FieldKind kind, double value) {
    std::ostringstream out;
    out << std::fixed;
    switch (kind) {
        case FieldKind::Mass:
            out << std::setprecision(4) << value;
            break;
        case FieldKind::X:
        case FieldKind::Y:
            out << std::setprecision(6) << value;
            break;
        case FieldKind::Vx:
        case FieldKind::Vy:
            out << std::setprecision(6) << value;
            break;
    }
    return out.str();
}

bool BodyEditor::parseValue(FieldKind kind, const std::string& text, double& out) {
    (void)kind;
    if (text.empty()) {
        return false;
    }
    try {
        std::size_t consumed = 0;
        const double parsed = std::stod(text, &consumed);
        if (consumed != text.size()) {
            return false;
        }
        out = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

void BodyEditor::syncFromBodies(const std::vector<MassPoint>& bodies) {
    layoutFields();
    for (auto& field : fields_) {
        if (field.body_index >= static_cast<int>(bodies.size())) {
            field.text.clear();
            continue;
        }
        const auto& body = bodies[field.body_index];
        switch (field.kind) {
            case FieldKind::Mass: field.text = formatValue(field.kind, body.mass); break;
            case FieldKind::X: field.text = formatValue(field.kind, body.x); break;
            case FieldKind::Y: field.text = formatValue(field.kind, body.y); break;
            case FieldKind::Vx: field.text = formatValue(field.kind, body.vx); break;
            case FieldKind::Vy: field.text = formatValue(field.kind, body.vy); break;
        }
    }
}

void BodyEditor::loadDefaults(const std::vector<MassPoint>& defaults) {
    syncFromBodies(defaults);
    focused_index_ = -1;
}

bool BodyEditor::applyToBodies(std::vector<MassPoint>& bodies) const {
    if (bodies.size() != static_cast<std::size_t>(BODY_COUNT)) {
        bodies.assign(BODY_COUNT, MassPoint{});
    }

    for (const auto& field : fields_) {
        double value = 0.0;
        if (!parseValue(field.kind, field.text, value)) {
            return false;
        }

        auto& body = bodies[field.body_index];
        body.id = field.body_index;
        body.name = "m" + std::to_string(field.body_index + 1);
        switch (field.kind) {
            case FieldKind::Mass: body.mass = value; break;
            case FieldKind::X: body.x = value; break;
            case FieldKind::Y: body.y = value; break;
            case FieldKind::Vx: body.vx = value; break;
            case FieldKind::Vy: body.vy = value; break;
        }
    }
    return true;
}

bool BodyEditor::contains(const sf::Vector2f& point) const {
    return point.x >= panel_left_ && point.x <= panel_left_ + panel_width_
        && point.y >= PANEL_TOP && point.y <= PANEL_TOP + PANEL_HEIGHT;
}

void BodyEditor::clearFocus() {
    focused_index_ = -1;
}

bool BodyEditor::handleMousePress(const sf::Vector2f& mouse) {
    for (std::size_t i = 0; i < fields_.size(); ++i) {
        if (fields_[i].rect.contains(mouse)) {
            focused_index_ = static_cast<int>(i);
            return true;
        }
    }
    focused_index_ = -1;
    return contains(mouse);
}

void BodyEditor::handleTextEntered(sf::Uint32 unicode) {
    if (focused_index_ < 0 || focused_index_ >= static_cast<int>(fields_.size())) {
        return;
    }
    if (unicode == 8 || unicode == 127) {
        return;
    }
    if (unicode == 13 || unicode == 10) {
        return;
    }
    if (unicode < 32 || unicode > 126) {
        return;
    }
    if (!isNumericChar(unicode)) {
        return;
    }
    fields_[focused_index_].text.push_back(static_cast<char>(unicode));
}

void BodyEditor::handleKeyPressed(sf::Keyboard::Key key) {
    if (focused_index_ < 0 || focused_index_ >= static_cast<int>(fields_.size())) {
        return;
    }
    if (key == sf::Keyboard::Backspace && !fields_[focused_index_].text.empty()) {
        fields_[focused_index_].text.pop_back();
    }
}

void BodyEditor::draw(sf::RenderTarget& target, const sf::Font& font) const {
    sf::RectangleShape panel({panel_width_, PANEL_HEIGHT});
    panel.setPosition(panel_left_, PANEL_TOP);
    panel.setFillColor(sf::Color(24, 24, 30, 240));
    panel.setOutlineColor(sf::Color(70, 70, 82));
    panel.setOutlineThickness(1.f);
    target.draw(panel);

    auto draw_button = [&](const sf::FloatRect& rect, const std::string& label, bool active) {
        sf::RectangleShape btn({rect.width, rect.height});
        btn.setPosition(rect.left, rect.top);
        btn.setFillColor(active ? sf::Color(70, 110, 170) : sf::Color(55, 55, 65));
        btn.setOutlineColor(sf::Color(90, 90, 100));
        btn.setOutlineThickness(1.f);
        target.draw(btn);

        sf::Text text(label, font, 13);
        text.setFillColor(sf::Color::White);
        const auto bounds = text.getLocalBounds();
        text.setPosition(rect.left + (rect.width - bounds.width) / 2.f - bounds.left,
                         rect.top + (rect.height - bounds.height) / 2.f - bounds.top);
        target.draw(text);
    };

    sf::Text title("Initial conditions", font, 15);
    title.setFillColor(sf::Color::White);
    title.setPosition(panel_left_ + PAD, PANEL_TOP + 10.f);
    target.draw(title);

    draw_button(apply_btn_, "Apply", false);
    draw_button(figure8_btn_, "Figure-8", false);
    draw_button(lagrange_btn_, "Lagrange", false);
    draw_button(collinear_btn_, "Collinear", false);

    const sf::Color body_colors[] = {
        sf::Color::Red, sf::Color::Green, sf::Color::Blue
    };

    for (int body = 0; body < BODY_COUNT; ++body) {
        const float block_top = PANEL_TOP + BODY_BLOCKS_TOP + body * BODY_BLOCK_H;

        sf::Text header("m" + std::to_string(body + 1), font, 14);
        header.setFillColor(body_colors[body]);
        header.setPosition(panel_left_ + PAD, block_top);
        target.draw(header);

        for (std::size_t i = 0; i < fields_.size(); ++i) {
            const auto& field = fields_[i];
            if (field.body_index != body) {
                continue;
            }

            sf::Text label(field.label, font, 12);
            label.setFillColor(sf::Color(180, 180, 190));
            label.setPosition(field.rect.left - LABEL_W, field.rect.top + 3.f);
            target.draw(label);

            const bool focused = static_cast<int>(i) == focused_index_;
            sf::RectangleShape box({field.rect.width, field.rect.height});
            box.setPosition(field.rect.left, field.rect.top);
            box.setFillColor(focused ? sf::Color(42, 42, 52) : sf::Color(34, 34, 42));
            box.setOutlineColor(focused ? sf::Color(110, 150, 210) : sf::Color(80, 80, 92));
            box.setOutlineThickness(1.f);
            target.draw(box);

            sf::Text value(field.text, font, 12);
            value.setFillColor(sf::Color::White);
            value.setPosition(field.rect.left + 4.f, field.rect.top + 3.f);
            target.draw(value);
        }
    }

    sf::Text hint("Click a field, type a number, then Apply", font, 11);
    hint.setFillColor(sf::Color(130, 130, 140));
    hint.setPosition(panel_left_ + PAD, PANEL_TOP + PANEL_HEIGHT - 24.f);
    target.draw(hint);
}

}
