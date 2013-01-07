#include "stdafx.h"
#include "core/test_server.h"
#include "core/test_client.h"
#include "core/logger.h"


using namespace core;
using namespace std::chrono;
namespace gl = oglplus;


int main(int argc, char **argv)
{
	std::locale::global(std::locale("rus"));
    LogService::ScopeGuard logGuard(&std::cout);

    try
    {
        sf::ContextSettings settings;
        settings.depthBits = 24;
        settings.stencilBits = 8;
        settings.antialiasingLevel = 4;
        settings.majorVersion = 3;
        settings.minorVersion = 0;

        sf::Window window(sf::VideoMode(800, 600), "OpenGL", sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close, settings);

        //window.setFramerateLimit(300);
        //window.setVerticalSyncEnabled(true);

        glewExperimental = GL_TRUE;
        glewInit();


        typedef gl::Context glContext;

        auto start_time = system_clock::now();
        size_t frameCount = 0;
        bool running = true;
        std::atomic<sf::Event> resized;

        std::thread eventThread([&]{
            while (running)
            {
                sf::Event evt;
                while (window.pollEvent(evt))
                {
                    switch (evt.type)
                    {
                    case sf::Event::Closed:
                        running = false;
                        break;

                    case sf::Event::KeyPressed:
                        if (evt.key.code == sf::Keyboard::Escape)
                            running = false;
                        break;
                    case sf::Event::Resized:
                        resized.store(evt);
                        break;
                    }
                }
            }
        });

        while (running)
        {
            LogDebug() << "poll";

            
            if (resized.load().type == sf::Event::Resized)
            {
                sf::Event evt = resized.exchange(sf::Event());
                // adjust the viewport when the window is resized
                glContext::Viewport(0, 0, evt.size.width, evt.size.height);
            }


            ++frameCount;
            
            float c = (frameCount % 10000) / 10000.f;
            glContext::ClearColor(c, c, c, 1.0f);
            glContext::ClearDepth(1.0);
            glContext::Clear().ColorBuffer().DepthBuffer();

            LogDebug() << "frame" << frameCount;
            window.display();

            sf::sleep(sf::milliseconds(50));
        }

        typedef duration<double, std::ratio<1>> d_seconds;
        auto elapsed = duration_cast<d_seconds>(system_clock::now() - start_time);
        LogDebug() << "fps:" << set_fixed(3) << frameCount / elapsed.count();

        window.close();

        return 0;

        IOServiceThread ioThread;

        const size_t maxTicks = argc > 2 ? atoi(argv[2]) : 10;
        const std::string mode = argc > 1 ? argv[1] : "server";

        if (mode == "server")
        {
            TestServer server(ioThread, 13999, maxTicks);
        }
        else
        {
            TestClient client(ioThread, 0, maxTicks);
            TestClient client2(ioThread, 0, maxTicks);
            //TestClient client3(ioThread, 0, maxTicks);
            //TestClient client4(ioThread, 0, maxTicks);
        }
    }
    catch (const std::exception& ex)
    {
        LogError() << ex.what();
    }

	return 0;
}
