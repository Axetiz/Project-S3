// Translate SFML 2/3 events into the same application input handlers.
// Keep version-specific event shapes here; editing and simulation decisions live in controllers.
#include "application.hpp"
namespace orbitsimlite {
void Application::poll_events(sf::RenderWindow &window) {
#if SFML_VERSION_MAJOR >= 3
    while (const auto event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>())
            dispatch({Action::Quit});
        else if (event->is<sf::Event::FocusLost>())
            mouse_release(0);
        else if (const auto *e = event->getIf<sf::Event::Resized>())
            world_.resize(e->size.x, e->size.y);
        else if (const auto *e = event->getIf<sf::Event::MouseMoved>())
            pointer(e->position);
        else if (const auto *e = event->getIf<sf::Event::MouseWheelScrolled>())
            wheel(e->delta);
        else if (const auto *e = event->getIf<sf::Event::MouseButtonPressed>()) {
            const int button =
                e->button == sf::Mouse::Button::Left ? 0 : (e->button == sf::Mouse::Button::Right ? 1 : 2);
            mouse_press(button, e->position);
        } else if (const auto *e = event->getIf<sf::Event::MouseButtonReleased>())
            mouse_release(int(e->button));
        else if (const auto *e = event->getIf<sf::Event::KeyPressed>())
            key(e->code, e->control, e->shift);
        else if (const auto *e = event->getIf<sf::Event::TextEntered>())
            text(e->unicode);
    }
#else
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            dispatch({Action::Quit});
        else if (event.type == sf::Event::LostFocus)
            mouse_release(0);
        else if (event.type == sf::Event::Resized)
            world_.resize(event.size.width, event.size.height);
        else if (event.type == sf::Event::MouseMoved)
            pointer({event.mouseMove.x, event.mouseMove.y});
        else if (event.type == sf::Event::MouseWheelScrolled)
            wheel(event.mouseWheelScroll.delta);
        else if (event.type == sf::Event::MouseButtonPressed)
            mouse_press(event.mouseButton.button == sf::Mouse::Left
                            ? 0
                            : (event.mouseButton.button == sf::Mouse::Right ? 1 : 2),
                        {event.mouseButton.x, event.mouseButton.y});
        else if (event.type == sf::Event::MouseButtonReleased)
            mouse_release(int(event.mouseButton.button));
        else if (event.type == sf::Event::KeyPressed)
            key(event.key.code, event.key.control, event.key.shift);
        else if (event.type == sf::Event::TextEntered)
            text(event.text.unicode);
    }
#endif
}
} // namespace orbitsimlite
