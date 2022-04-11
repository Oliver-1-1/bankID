#include <SFML/Graphics.hpp>

#include "bankID.h"

int main() {
  
    bankID::init();
    bankID::auth("202203072380");
	
    sf::RenderWindow window(sf::VideoMode(300, 300), "SFML window");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)){
            if (event.type == sf::Event::Closed)
                window.close();
        }

        bankID::qrCode::update_time();
        bankID::collect("");

        sf::Texture texture;
        texture.loadFromFile("qrCode.png");
        sf::Sprite sprite(texture);

        window.clear();
        window.draw(sprite);
        window.display();
       
    }
    return 0;
}

