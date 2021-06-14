// Internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "ModelFactories.h"
#include "Camera.h"
#include "Font.h"

// External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <random>
#include <il.h>
#include <glm/gtx/vector_angle.hpp>
#include <list>

// Window size
static const GLsizei WIDTH = 1280, HEIGHT = 720;

int initGL() {
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    return 0;
}

static double mx = 0;
static double my = 0;
constexpr float scale = 1.0f;
const glm::vec3 forward(0.0f, 0.0f, -scale);
const glm::vec3 left(-scale, 0.0f, 0.0f);
const glm::vec3 up(0.0f, scale, 0.0f);

// Settings
static bool permitMouseMove = false;

// Prepare transformations
const auto perspective = glm::perspective(glm::radians(45.0f), float(WIDTH) / HEIGHT, 0.1f, 1000.0f);

static float yaw = 0.0;
static float pitch = 0.0;
// Callback for mouse movement
static void mouseMove(GLFWwindow *window, double xpos, double ypos) {
    auto x1 = float(0.01 * xpos);
    auto y1 = float(0.01 * ypos);

    if (permitMouseMove) {
        yaw = glm::clamp(yaw + float(my - y1), -M_PI_2f32, M_PI_2f32);
        pitch += mx - x1;
//        camera_rot = glm::rotate(glm::mat4(1.0f), float(my - y1), glm::vec3(1.0f, 0.0f, 0.0f)) *
//                     glm::rotate(glm::mat4(1.0f), float(mx - x1), glm::vec3(0.0f, 1.0f, 0.0f)) * camera_rot;
    }

    mx = x1;
    my = y1;
}

static bool shoot = false;
// Callback for actions with mouse buttons
// Permit camera movements with left button pressed only
static void mouseButton(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            permitMouseMove = true;

        } else if (action == GLFW_RELEASE) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            permitMouseMove = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
           shoot = true;
        }
    }
}

// Callback for movement controls
// W - forward
// A - left
// S - backward
// D - right
// Q - up
// E - down
// SPACE - reset position
float multiplier = 0.1f;
glm::vec3 step = {0.0f, 0.0f, 0.0f};
CameraMode camera_mode = CameraMode::THIRD_PERSON;
static void keyboardControls(GLFWwindow *window, int key, int scancode, int action, int mods) {
    switch (key) {
        case GLFW_KEY_W:
            if (action == GLFW_PRESS) {
                step += forward;
            } else if (action == GLFW_RELEASE) {
                step -= forward;
            }
            break;
        case GLFW_KEY_A:
            if (action == GLFW_PRESS) {
                step += left;
            } else if (action == GLFW_RELEASE) {
                step -= left;
            }
            break;
        case GLFW_KEY_S:
            if (action == GLFW_PRESS) {
                step -= forward;
            } else if (action == GLFW_RELEASE) {
                step += forward;
            }
            break;
        case GLFW_KEY_D:
            if (action == GLFW_PRESS) {
                step -= left;
            } else if (action == GLFW_RELEASE) {
                step += left;
            }
            break;
        case GLFW_KEY_R:
            if (action == GLFW_PRESS) {
                step += up;
            } else if (action == GLFW_RELEASE) {
                step -= up;
            }
            break;
        case GLFW_KEY_F:
            if (action == GLFW_PRESS) {
                step -= up;
            } else if (action == GLFW_RELEASE) {
                step += up;
            }
            break;
        case GLFW_KEY_LEFT_SHIFT:
        case GLFW_KEY_RIGHT_SHIFT:
            if (action == GLFW_PRESS) {
                multiplier *= 2;
            } else if (action == GLFW_RELEASE) {
                multiplier /= 2;
            }
            break;
        case GLFW_KEY_F2:
            if (action == GLFW_PRESS) {
                camera_mode = CameraMode::FIRST_PERSON;
            }
            break;
        case GLFW_KEY_F3:
            if (action == GLFW_PRESS) {
                camera_mode = CameraMode::THIRD_PERSON;
            }
            break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        default:
            break;
    }
}

enum class ShaderType {
    CLASSIC,
    SKYBOX,
    PARTICLES,
    CROSSHAIR,
    EXPLOSION,
    TEXT,
    LASER,
};

int main(int argc, char **argv) {
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL basic sample", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseMove);
    glfwSetMouseButtonCallback(window, mouseButton);
    glfwSetKeyCallback(window, keyboardControls);

    if (initGL() != 0)
        return -1;

    ilInit();

    // Reset any OpenGL errors which could be present for some reason
    GLenum gl_error = glGetError();
    while (gl_error != GL_NO_ERROR)
        gl_error = glGetError();

    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    GL_CHECK_ERRORS;

    std::cout << "Initializing environment... ";
    std::cout << "\x1b[32mDone\x1b[0m" << std::endl;

    std::cout << "Compiling shaders... ";

    std::unordered_map<ShaderType, ShaderProgram> shader_programs;
    shader_programs[ShaderType::CLASSIC] = ShaderProgram({
        {GL_VERTEX_SHADER,   "shaders/classic_vertex.glsl"},
        {GL_FRAGMENT_SHADER, "shaders/classic_fragment.glsl"},
    });
    shader_programs[ShaderType::SKYBOX] = ShaderProgram({
        {GL_VERTEX_SHADER,   "shaders/skybox_vertex.glsl"},
        {GL_FRAGMENT_SHADER, "shaders/skybox_fragment.glsl"},
    });
    shader_programs[ShaderType::PARTICLES] = ShaderProgram({
        {GL_VERTEX_SHADER,   "shaders/particles_vertex.glsl"},
        {GL_GEOMETRY_SHADER, "shaders/particles_geometry.glsl"},
        {GL_FRAGMENT_SHADER, "shaders/particles_fragment.glsl"},
    });
    shader_programs[ShaderType::CROSSHAIR] = ShaderProgram({
        {GL_VERTEX_SHADER,   "shaders/crosshair_vertex.glsl"},
        {GL_FRAGMENT_SHADER, "shaders/crosshair_fragment.glsl"},
    });
    shader_programs[ShaderType::EXPLOSION] = ShaderProgram({
        {GL_VERTEX_SHADER,   "shaders/explosion_vertex.glsl"},
        {GL_GEOMETRY_SHADER, "shaders/explosion_geometry.glsl"},
        {GL_FRAGMENT_SHADER, "shaders/explosion_fragment.glsl"},
    });
    shader_programs[ShaderType::TEXT] = ShaderProgram({
        {GL_VERTEX_SHADER,   "shaders/text_vertex.glsl"},
        {GL_FRAGMENT_SHADER, "shaders/text_fragment.glsl"},
    });
    shader_programs[ShaderType::LASER] = ShaderProgram({
        {GL_VERTEX_SHADER,   "shaders/laser_vertex.glsl"},
        {GL_FRAGMENT_SHADER, "shaders/laser_fragment.glsl"},
    });
    GL_CHECK_ERRORS;

    std::cout << "\x1b[32mDone\x1b[0m" << std::endl;

    Camera camera;

    std::cout << "Loading skybox... ";

    const auto skybox = SkyBox::create({
        "models/necro_nebula/little_GalaxyTex_PositiveX.png",
        "models/necro_nebula/little_GalaxyTex_NegativeX.png",
        "models/necro_nebula/little_GalaxyTex_NegativeY.png",
        "models/necro_nebula/little_GalaxyTex_PositiveY.png",
        "models/necro_nebula/little_GalaxyTex_PositiveZ.png",
        "models/necro_nebula/little_GalaxyTex_NegativeZ.png",
    });

    std::cout << "\x1b[32mDone\x1b[0m" << std::endl;

    Particles particles(1000);

    Crosshair crosshair;

    Laser laser;
    constexpr int laser_recharge_rate = 15;

    std::cout << "Loading models... ";

    ModelFactory model_factory;

    std::cout << "\x1b[32mDone\x1b[0m" << std::endl;

    int score = 0;
    float main_ship_hp = 100.0;
    auto main_ship = model_factory.get_model(ModelName::E45_AIRCRAFT);

    std::list<Model> enemies;

    std::list<Asteroid> asteroids;

    Font font("models/arial.ttf");

    glfwSwapInterval(1); // force 60 frames per second

    glm::vec3 smooth_step(0.0f);
    glm::vec3 enemies_speed(0.0f, 0.0f, 0.0f);
    glm::vec3 particles_state(0.0f, 0.0f, 0.0f);
    float speed_multiplier = 1.0f;

    glm::vec3 laser_dst;

    const glm::vec4 view_port(0.0f, 0.0f, WIDTH, HEIGHT);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Game loop
    while (!glfwWindowShouldClose(window)) {
        // Tech stuff
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GL_CHECK_ERRORS;

        // Game logic

        // Modify environment
        const auto time = glfwGetTime();

        smooth_step += 0.05f * (step - smooth_step);
        const auto camera_shift = multiplier * smooth_step;
        camera.mode = camera_mode;
        camera.rot = glm::quat({yaw, pitch, 0.0f});
        camera.move(camera_shift);

        const auto view_transform = camera.getViewTransform();
        const auto perspective_transform = perspective * view_transform;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (main_ship.dead) {
            shoot = false;
        }

        // Check laser status
        if (laser.recharge != 0) {
            shoot = false;
            laser.recharge--;
            laser_dst += enemies_speed + camera_shift;
        } else if (shoot) {
            laser.recharge = laser_recharge_rate;
        }

        // Kill targets
        if (shoot) {

            for (auto &enemy : enemies) {
                if (enemy.dead) continue;

                const auto model = view_transform * enemy.getWorldTransform();
                const auto bbox = enemy.bbox;
                float min_x = INFINITY;
                float min_y = INFINITY;
                float max_x = -INFINITY;
                float max_y = -INFINITY;

                for (const auto pos : {glm::vec3(bbox.min.x, bbox.min.y, bbox.min.z),
                                       glm::vec3(bbox.min.x, bbox.min.y, bbox.max.z),
                                       glm::vec3(bbox.min.x, bbox.max.y, bbox.min.z),
                                       glm::vec3(bbox.min.x, bbox.max.y, bbox.max.z),
                                       glm::vec3(bbox.max.x, bbox.min.y, bbox.min.z),
                                       glm::vec3(bbox.max.x, bbox.min.y, bbox.max.z),
                                       glm::vec3(bbox.max.x, bbox.max.y, bbox.min.z),
                                       glm::vec3(bbox.max.x, bbox.max.y, bbox.max.z)}) {

                    const auto tmp = glm::project(pos, model, perspective, view_port);

                    min_x = glm::min(min_x, tmp.x);
                    min_y = glm::min(min_y, tmp.y);
                    max_x = glm::max(max_x, tmp.x);
                    max_y = glm::max(max_y, tmp.y);
                }

                if (xpos >= min_x && xpos <= max_x &&
                    HEIGHT - ypos >= min_y && HEIGHT - ypos <= max_y) {

                    laser_dst = enemy.world_pos;
                    enemy.dead = true;
                    shoot = false;
                    score += enemy.damage;
                    break;
                }
            }
        }

        if (shoot) {
            for (auto& asteroid : asteroids) {
                if (asteroid.dead) continue;

                const auto model = view_transform * asteroid.getWorldTransform();
                const auto bbox = asteroid.bbox;
                float min_x = INFINITY;
                float min_y = INFINITY;
                float max_x = -INFINITY;
                float max_y = -INFINITY;

                for (const auto pos : {glm::vec3(bbox.min.x, bbox.min.y, bbox.min.z),
                                       glm::vec3(bbox.min.x, bbox.min.y, bbox.max.z),
                                       glm::vec3(bbox.min.x, bbox.max.y, bbox.min.z),
                                       glm::vec3(bbox.min.x, bbox.max.y, bbox.max.z),
                                       glm::vec3(bbox.max.x, bbox.min.y, bbox.min.z),
                                       glm::vec3(bbox.max.x, bbox.min.y, bbox.max.z),
                                       glm::vec3(bbox.max.x, bbox.max.y, bbox.min.z),
                                       glm::vec3(bbox.max.x, bbox.max.y, bbox.max.z)}) {

                    const auto tmp = glm::project(pos, model, perspective, view_port);

                    min_x = glm::min(min_x, tmp.x);
                    min_y = glm::min(min_y, tmp.y);
                    max_x = glm::max(max_x, tmp.x);
                    max_y = glm::max(max_y, tmp.y);
                }

                if (xpos >= min_x && xpos <= max_x &&
                    HEIGHT - ypos >= min_y && HEIGHT - ypos <= max_y) {

                    laser_dst = asteroid.world_pos;
                    asteroid.dead = true;
                    shoot = false;
                    score += asteroid.damage;
                    break;
                }
            }
        }

        if (shoot) {
            const auto tmp = glm::inverse(perspective_transform) * glm::vec4(2.0 * xpos / WIDTH - 1.0, -2.0 * ypos / HEIGHT + 1.0, 1.0f, 1.0f);
            laser_dst = glm::vec3(tmp) / tmp.w;
            shoot = false;
        }

        // Process enemies
        for (auto it = enemies.begin(); it != enemies.end();) {
            if (!it->dead) {
                if (intersect(main_ship.getBBox(), it->getBBox())) {
                    main_ship_hp = std::max(main_ship_hp - it->damage, 0.0f);
                    it->dead = true;
                } else if (it->world_pos.z > 200.0f) {
                    enemies.erase(it++);
                    continue;
                }

                // Shoot
                if (rand() % 1000 == 0) {
                    asteroids.emplace_back(model_factory.get_model(ModelName::ROCKET, it->world_pos),
                        speed_multiplier * 2.0f * glm::normalize(main_ship.world_pos - it->world_pos));
                }
            } else {
                if (it->die()) {
                    enemies.erase(it++);
                    continue;
                }
            }
            it->move(speed_multiplier * enemies_speed);
            it++;
        }

        // Process asteroids
        for (auto it = asteroids.begin(); it != asteroids.end();){
            if (!it->dead) {
                if (intersect(main_ship.getBBox(), it->getBBox())) {
                    main_ship_hp = std::max(main_ship_hp - it->damage, 0.0f);
                    it->dead = true;
                } else if (it->world_pos.z > 200.0f) {
                    asteroids.erase(it++);
                    continue;
                }
            } else {
                if (it->die()) {
                    asteroids.erase(it++);
                    continue;
                }
            }
            it->moveAuto(speed_multiplier);
            it++;
        }

        // Enemies spawn
        if (rand() % 300 == -1) {
            enemies.push_back(model_factory.get_random_enemy(camera.position));
            enemies.back().damage = 50.0f;
        }

        // Asteroids spawn
        if (rand() % 300 == -1) {
            asteroids.push_back(model_factory.get_random_asteroid(camera.position, main_ship.world_pos));
            asteroids.back().damage = 25.0f;
        }

        main_ship.move(camera_shift);
        if (main_ship_hp == 0.0f) {
            main_ship.dead = true;
        }

        particles_state += speed_multiplier * enemies_speed;

        speed_multiplier += 0.0001f;

        // Drawing

        // Draw skybox
        {
            auto& program = shader_programs[ShaderType::SKYBOX];

            glDepthMask(GL_FALSE);
            program.StartUseShader();

            const auto transform = perspective * glm::mat4(glm::mat3(view_transform));
            program.SetUniform("transform", transform);

            skybox.draw();

            program.StopUseShader();
            glDepthMask(GL_TRUE);
        }

        // Draw particles
        {
            auto& program = shader_programs[ShaderType::PARTICLES];

            program.StartUseShader();

            program.SetUniform("world_transform", glm::translate(glm::mat4(1.0f), particles_state - camera.position));
            program.SetUniform("perspective_transform", perspective * glm::mat4(glm::mat3(view_transform)));

            program.SetUniform("velocity", enemies_speed - camera_shift);

            particles.draw();

            program.StopUseShader();
        }

        // Draw objects
        {
            auto& program = shader_programs[ShaderType::CLASSIC];
            program.StartUseShader();
            GL_CHECK_ERRORS;

            // Draw enemies
            for (const auto &model : enemies) {
                if (model.dead) continue;
                const auto local = perspective_transform * model.getWorldTransform();
                for (const auto &object : model.objects) {
                    const auto transform = local * object.getWorldTransform();
                    program.SetUniform("transform", transform);

                    const auto color = object.getDiffuseColor();
                    program.SetUniform("diffuse_color", color);

                    const bool use_texture = object.haveTexture();
                    program.SetUniform("use_texture", use_texture);

                    const float opacity = object.getOpacity();
                    program.SetUniform("opacity", opacity);

                    object.draw();
                    GL_CHECK_ERRORS;
                }
            }

            // Draw asteroids
            for (const auto &model : asteroids) {
                if (model.dead) continue;
                const auto local = perspective_transform * model.getWorldTransform();
                for (const auto &object : model.objects) {
                    const auto transform = local * object.getWorldTransform();
                    program.SetUniform("transform", transform);

                    const auto color = object.getDiffuseColor();
                    program.SetUniform("diffuse_color", color);

                    const bool use_texture = object.haveTexture();
                    program.SetUniform("use_texture", use_texture);

                    const float opacity = object.getOpacity();
                    program.SetUniform("opacity", opacity);

                    object.draw();
                    GL_CHECK_ERRORS;
                }
            }

            program.StopUseShader();
        }

        // Draw dead objects
        {
            auto& program = shader_programs[ShaderType::EXPLOSION];

            program.StartUseShader();

            // Draw enemies
            for (const auto &model : enemies) {
                if (!model.dead) continue;
                const auto local = perspective_transform * model.getWorldTransform();
                const auto death_coef = float(model.death_countdown) / 60;
                program.SetUniform("magnitude", (1.0f - death_coef) * 5.0f);
                for (const auto &object : model.objects) {
                    const auto transform = local * object.getWorldTransform();
                    program.SetUniform("transform", transform);

                    const auto color = object.getDiffuseColor();
                    program.SetUniform("diffuse_color", color);

                    const bool use_texture = object.haveTexture();
                    program.SetUniform("use_texture", use_texture);

                    const float opacity = death_coef * object.getOpacity();
                    program.SetUniform("opacity", opacity);

                    object.draw();
                    GL_CHECK_ERRORS;
                }
            }

            // Draw asteroids
            for (const auto &model : asteroids) {
                if (!model.dead) continue;
                const auto local = perspective_transform * model.getWorldTransform();
                const auto death_coef = float(model.death_countdown) / 60;
                program.SetUniform("magnitude", (1.0f - death_coef) * 5.0f);
                for (const auto &object : model.objects) {
                    const auto transform = local * object.getWorldTransform();
                    program.SetUniform("transform", transform);

                    const auto color = object.getDiffuseColor();
                    program.SetUniform("diffuse_color", color);

                    const bool use_texture = object.haveTexture();
                    program.SetUniform("use_texture", use_texture);

                    const float opacity = death_coef * object.getOpacity();
                    program.SetUniform("opacity", opacity);

                    object.draw();
                    GL_CHECK_ERRORS;
                }
            }

            program.StopUseShader();
        }

        // Draw laser
        if (laser.recharge != 0) {
            auto& program = shader_programs[ShaderType::LASER];

            program.StartUseShader();

            program.SetUniform("transform", perspective_transform);

            glLineWidth(5.0f * float(laser.recharge) / laser_recharge_rate);
            laser.draw(main_ship.world_pos, laser_dst);
            glLineWidth(1.0f);

            program.StopUseShader();
        }

        // Main ship
        if (!main_ship.dead) {
            auto& program = shader_programs[ShaderType::CLASSIC];
            program.StartUseShader();

            const auto local = perspective_transform * main_ship.getWorldTransform();
            for (const auto &object : main_ship.objects) {
                const auto transform = local * object.getWorldTransform();
                program.SetUniform("transform", transform);

                const auto color = object.getDiffuseColor();
                program.SetUniform("diffuse_color", color);

                const bool use_texture = object.haveTexture();
                program.SetUniform("use_texture", use_texture);

                const float opacity = object.getOpacity();
                program.SetUniform("opacity", opacity);

                object.draw();
                GL_CHECK_ERRORS;
            }
            program.StopUseShader();

        } else if (!main_ship.die()) {
            auto& program = shader_programs[ShaderType::EXPLOSION];
            program.StartUseShader();

            const auto local = perspective_transform * main_ship.getWorldTransform();
            const auto death_coef = float(main_ship.death_countdown) / 60;
            program.SetUniform("magnitude", (1.0f - death_coef) * 5.0f);
            for (const auto &object : main_ship.objects) {
                const auto transform = local * object.getWorldTransform();
                program.SetUniform("transform", transform);

                const auto color = object.getDiffuseColor();
                program.SetUniform("diffuse_color", color);

                const bool use_texture = object.haveTexture();
                program.SetUniform("use_texture", use_texture);

                const float opacity = death_coef * object.getOpacity();
                program.SetUniform("opacity", opacity);

                object.draw();
                GL_CHECK_ERRORS;
            }

            program.StopUseShader();
        }

        // Draw crosshair
        {
            auto& program = shader_programs[ShaderType::CROSSHAIR];

            program.StartUseShader();

            program.SetUniform("position", glm::vec2(2.0 * xpos / WIDTH - 1.0, -2.0 * ypos / HEIGHT + 1.0));

            crosshair.draw();

            program.StopUseShader();
        }

        // Draw text
        {
            auto& program = shader_programs[ShaderType::TEXT];

            program.StartUseShader();

            const auto transform = glm::ortho(0.0f, float(WIDTH), 0.0f, float(HEIGHT));

            // Health Points
            program.SetUniform("transform", glm::translate(transform, {5.0f, 5.0f, 0.0f}));
            program.SetUniform("text_color", glm::vec3(1.0f, 0.0f, 0.0f));
            font.draw("Health: " + std::to_string(int(main_ship_hp)));

            // Score
            program.SetUniform("transform", glm::translate(transform, {5.0f, 45.0f, 0.0f}));
            program.SetUniform("text_color", glm::vec3(1.0f, 0.5f, 0.0f));
            font.draw("Score: " + std::to_string(score));

            // Game Over
            if (main_ship.dead) {
                program.SetUniform("transform", glm::translate(transform, {WIDTH / 2.0f - 100.0f, HEIGHT / 2.0f + 40.f, 0.0f}));
                program.SetUniform("text_color", glm::vec3(1.0f, 1.0f, 1.0f));
                font.draw("Game Over");

                program.SetUniform("transform", glm::scale(glm::translate(transform, {WIDTH / 2.0f - 140.0f, HEIGHT / 2.0f - 40.f, 0.0f}), {0.75f, 0.75f, 0.75f}));
                program.SetUniform("text_color", glm::vec3(1.0f, 1.0f, 1.0f));
                font.draw("Press ESC to leave");
            }

            program.StopUseShader();
            GL_CHECK_ERRORS;
        }

        glfwSwapBuffers(window);

    }
    std::cout << "\nGame Over!" << std::endl;

    glfwTerminate();
    return 0;
}
