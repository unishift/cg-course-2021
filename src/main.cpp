// Internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "ModelFactories.h"
#include "Camera.h"
#include "Font.h"
#include "ShadowMap.h"

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
const auto perspective = glm::perspective(glm::radians(45.0f), float(WIDTH) / HEIGHT, 0.1f, 80.0f);

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

enum class ShaderType {
    CLASSIC,
    SKYBOX,
    PARTICLES,
    DEPTH
};

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
CameraMode camera_mode = CameraMode::FIRST_PERSON;
ShaderType main_shader = ShaderType::CLASSIC;
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
        case GLFW_KEY_1:
            if (action == GLFW_PRESS) {
                main_shader = ShaderType::CLASSIC;
            }
            break;
        case GLFW_KEY_2:
            if (action == GLFW_PRESS) {
                main_shader = ShaderType::DEPTH;
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

class Game {
public:
    GLFWwindow *window;

    std::unordered_map<ShaderType, ShaderProgram> shader_programs;
    Camera camera;
    SkyBox skybox;
    Particles particles;
    Crosshair crosshair;
    Laser laser;
    const int laser_recharge_rate = 15;
    ModelFactory model_factory;

    int score = 0;
    float main_ship_hp = 100.0;
    std::list<Model> enemies;
    std::list<Asteroid> asteroids;

    Font font;

    glm::vec3 smooth_step = glm::vec3(0.0f);
    glm::vec3 enemies_speed = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 particles_state = glm::vec3(0.0f, 0.0f, 0.0f);
    float speed_multiplier = 1.0f;

    glm::vec3 laser_dst;

    const glm::vec4 view_port = glm::vec4(0.0f, 0.0f, WIDTH, HEIGHT);

    Game() = default;

    int init_GL() {
        if (!glfwInit())
            return -1;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Lights and Shadows", nullptr, nullptr);
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
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GL_CHECK_ERRORS;

        return 0;
    }

    void compile_shaders() {
        shader_programs[ShaderType::CLASSIC] = ShaderProgram({
            {GL_VERTEX_SHADER,   "shaders/classic/classic_vertex.glsl"},
            {GL_FRAGMENT_SHADER, "shaders/classic/classic_fragment.glsl"},
        });
        GL_CHECK_ERRORS;

        shader_programs[ShaderType::SKYBOX] = ShaderProgram({
            {GL_VERTEX_SHADER,   "shaders/skybox/skybox_vertex.glsl"},
            {GL_FRAGMENT_SHADER, "shaders/skybox/skybox_fragment.glsl"},
        });
        GL_CHECK_ERRORS;

        shader_programs[ShaderType::PARTICLES] = ShaderProgram({
            {GL_VERTEX_SHADER,   "shaders/particles/particles_vertex.glsl"},
            {GL_GEOMETRY_SHADER, "shaders/particles/particles_geometry.glsl"},
            {GL_FRAGMENT_SHADER, "shaders/particles/particles_fragment.glsl"},
        });
        GL_CHECK_ERRORS;

        shader_programs[ShaderType::DEPTH] = ShaderProgram({
            {GL_VERTEX_SHADER,   "shaders/depth/depth_vertex.glsl"},
            {GL_FRAGMENT_SHADER, "shaders/depth/depth_fragment.glsl"},
        });
        GL_CHECK_ERRORS;
    }

    void load_skybox() {
        skybox = SkyBox::create({
            "models/necro_nebula/little_GalaxyTex_PositiveX.png",
            "models/necro_nebula/little_GalaxyTex_NegativeX.png",
            "models/necro_nebula/little_GalaxyTex_NegativeY.png",
            "models/necro_nebula/little_GalaxyTex_PositiveY.png",
            "models/necro_nebula/little_GalaxyTex_PositiveZ.png",
            "models/necro_nebula/little_GalaxyTex_NegativeZ.png",
        });
    }

    void init_objects() {
        // Main ship
        enemies.push_back(model_factory.get_model(ModelName::E45_AIRCRAFT, glm::vec3(3.6, 1.9, -31.9)));

        // Stationary object
        enemies.push_back(model_factory.get_model(ModelName::REPVENATOR, glm::vec3(0., 0., -50.)));

        // Moving object
        enemies.push_back(model_factory.get_model(ModelName::ASTEROID1, glm::vec3(10., 5., -35.)));
    }

    int init() {
        int return_code;

        std::cout << "Initializing environment... ";
        return_code = init_GL();

        if (return_code != 0) {
            return return_code;
        }
        std::cout << "\x1b[32mDone\x1b[0m" << std::endl;

        std::cout << "Compiling shaders... ";
        compile_shaders();
        std::cout << "\x1b[32mDone\x1b[0m" << std::endl;

        std::cout << "Loading skybox... ";
        load_skybox();
        std::cout << "\x1b[32mDone\x1b[0m" << std::endl;

        std::cout << "Loading models... ";
        model_factory.load();
        std::cout << "\x1b[32mDone\x1b[0m" << std::endl;

        font = Font("models/arial.ttf");
        crosshair.init();
        laser.init();
        particles = Particles(1000);

        init_objects();

        glfwSwapInterval(1); // force 60 frames per second

        glEnable(GL_MULTISAMPLE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);

        return 0;
    }

    // Game mechanics

    double xpos, ypos;
    glm::mat4 view_transform;
    glm::mat4 perspective_transform;
    glm::vec3 camera_shift;

    void modify_env() {
        const auto time = glfwGetTime();

        smooth_step += 0.05f * (step - smooth_step);
        camera_shift = multiplier * smooth_step;
        camera.mode = camera_mode;
        camera.rot = glm::quat({yaw, pitch, 0.0f});
        camera.move(camera_shift);

        view_transform = camera.getViewTransform();
        perspective_transform = perspective * view_transform;

        glfwGetCursorPos(window, &xpos, &ypos);
    }

    void draw_skybox() {
        auto& program = shader_programs[ShaderType::SKYBOX];

        glDepthMask(GL_FALSE);
        program.StartUseShader();

        const auto transform = perspective * glm::mat4(glm::mat3(view_transform));
        program.SetUniform("transform", transform);

        skybox.draw();

        program.StopUseShader();
        glDepthMask(GL_TRUE);
    }

    void draw_particles() {
        auto& program = shader_programs[ShaderType::PARTICLES];

        program.StartUseShader();

        program.SetUniform("world_transform", glm::translate(glm::mat4(1.0f), particles_state - camera.position));
        program.SetUniform("perspective_transform", perspective * glm::mat4(glm::mat3(view_transform)));

        program.SetUniform("velocity", enemies_speed - camera_shift);

        particles.draw();

        program.StopUseShader();
    }

    void draw_objects(const glm::mat4& depth_matrix)
    {
        auto& program = shader_programs[ShaderType::CLASSIC];
        program.StartUseShader();
        GL_CHECK_ERRORS;

        // Draw enemies
        for (const auto &model : enemies) {
            if (model.dead) continue;

            const auto local = perspective_transform * model.getWorldTransform();
            const auto depth = depth_matrix * model.getWorldTransform();

            for (const auto &object : model.objects) {
                const auto transform = local * object.getWorldTransform();
                program.SetUniform("transform", transform);

                const auto depth_transform = depth * object.getWorldTransform();
                program.SetUniform("depth_transform", depth_transform);

                const auto color = object.getDiffuseColor();
                program.SetUniform("diffuse_color", color);

                const bool use_texture = object.haveTexture();
                program.SetUniform("use_texture", use_texture);

                const float opacity = object.getOpacity();
                program.SetUniform("opacity", opacity);

                const auto light_direction = glm::vec3(-15.f, -15.f, -35.f);
                program.SetUniform("light_direction", -light_direction);

                program.SetUniform("Texture", 0);
                program.SetUniform("shadow_map", 1);

                object.draw();
                GL_CHECK_ERRORS;
            }
        }
        program.StopUseShader();
    }

    void draw_depth(const glm::mat4& matrix) {
        auto& program = shader_programs[ShaderType::DEPTH];
        program.StartUseShader();
        GL_CHECK_ERRORS;

        // Draw enemies
        for (const auto &model : enemies) {
            if (model.dead) continue;
            const auto local = matrix * model.getWorldTransform();
            for (const auto &object : model.objects) {
                const auto transform = local * object.getWorldTransform();
                program.SetUniform("transform", transform);

                object.draw();
                GL_CHECK_ERRORS;
            }
        }
        program.StopUseShader();
    }

    int game_loop() {
        auto& main_ship = enemies.front();

        camera.move({4, 4, 0});

        auto& asteroid = enemies.back();
        float asteroid_state = 0.f;
        constexpr float asteroid_step = 2 * M_PI / 60 / 10;  // Round every 10 seconds
        const glm::vec3 asteroid_center = enemies.back().world_pos;

        // Shadows

        ShadowMap shadow_map(
            glm::ortho<float>(-30, 30, -20, 20, 10, 80)
          * glm::lookAt(glm::vec3(12, 12, -12), glm::vec3(0, 0, -40), glm::vec3(0, 1, 0))
        );
        shadow_map.init(WIDTH, HEIGHT);

        while (!glfwWindowShouldClose(window)) {
            // Tech stuff
            glfwPollEvents();

            glViewport(0, 0, WIDTH, HEIGHT);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            GL_CHECK_ERRORS;

            // Game logic

            modify_env();

            particles_state += speed_multiplier * enemies_speed;
            speed_multiplier += 0.0001f;

            asteroid.world_pos = asteroid_center + 10.f * glm::vec3(sinf(asteroid_state), 0.f, cosf(asteroid_state));
            asteroid_state += asteroid_step;

            // Drawing

            shadow_map.activate();
            draw_depth(shadow_map.matrix);
            shadow_map.deactivate();

            const glm::mat4 bias(
                0.5, 0.0, 0.0, 0.0,
                0.0, 0.5, 0.0, 0.0,
                0.0, 0.0, 0.5, 0.0,
                0.5, 0.5, 0.5, 1.0
            );
            const glm::mat4 depth_matrix = bias * shadow_map.matrix;

            draw_skybox();
            draw_particles();

            shadow_map.bind();
            draw_objects(depth_matrix);

            if (main_shader == ShaderType::DEPTH) {
                // Show depth only in part of the screen
                glViewport(0, 0, WIDTH / 3, HEIGHT / 3);

                glScissor(0, 0, WIDTH / 3, HEIGHT / 3);
                glEnable(GL_SCISSOR_TEST);
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glDisable(GL_SCISSOR_TEST);

                draw_depth(shadow_map.matrix);
            }

            glfwSwapBuffers(window);
        }
        std::cout << "\nGame Over!" << std::endl;

        glfwTerminate();

        return 0;
    }
};

int main(int argc, char **argv) {
    Game game;
    int init_code = game.init();
    if (init_code != 0) {
        return init_code;
    }

    return game.game_loop();
}
