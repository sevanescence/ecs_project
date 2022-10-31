#include <unordered_map>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include <format>

#include <cmath>

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/intersect.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

constexpr int glfw_version_major = 3;
constexpr int glfw_version_minor = 3;

// log() is easier to type than puts() and it's usage is better implied
void log(const char *msg) {
    std::cout << msg << '\n';
}

namespace mk {
    class gl_camera {
    public:
        enum direction {
            FRONT,
            BACK,
            FRONT_LEFT,
            FRONT_RIGHT
        };

        gl_camera()
            : pos{}, front{ 0.0f, 0.0f, -1.0f }, up{ 0.0f, 1.0f, 0.0f }, 
            field_of_view{ 45.0f }, near{ 0.1f }, far{ 1000.0f }, movement_enabled{ true } {
            set_rotation(0, 0);
        }
        gl_camera(glm::vec3 camera_pos)
            : pos(camera_pos), front{ 0.0f, 0.0f, -1.0f }, up{ 0.0f, 1.0f, 0.0f }, 
            field_of_view{ 45.0f }, near{ 0.1f }, far{ 1000.0f }, movement_enabled{ true } {
            set_rotation(0, 0);
        }

        void set_rotation(direction camera_direction) {
            switch (camera_direction) {
            case direction::FRONT:
                set_rotation(0, 0);
                break;
            case direction::BACK:
                set_rotation(-90, 0);
                break;
            case direction::FRONT_LEFT:
                set_rotation(-45, 0);
                break;
            case direction::FRONT_RIGHT:
                set_rotation(45, 0);
                break;
            }
        }

        void set_rotation(double pitch, double yaw) {
            front 
                = glm::normalize(glm::vec3(
                sin(glm::radians(yaw)) * cos(glm::radians(pitch)),
                sin(glm::radians(pitch)),
                -cos(glm::radians(yaw)) * cos(glm::radians(pitch))
            ));
            m_pitch = pitch;
            m_yaw = yaw;
            m_velocity = glm::normalize(glm::vec3{ front.x, 0.0f, front.z });
        }

        float get_pitch() const noexcept { return m_pitch; }
        float get_yaw() const noexcept { return m_yaw; }
        glm::vec3 get_velocity() const noexcept { return m_velocity; }

        glm::mat4 get_view() const {
            return glm::lookAt(pos, pos + front, up);
        }

        glm::mat4 get_perspective() const {
            return glm::perspective(glm::radians(field_of_view), aspect, near, far);
        }

        glm::vec3 pos;
        glm::vec3 front;
        glm::vec3 up;
        float field_of_view;
        float aspect;
        float near;
        float far;

        bool movement_enabled;
        float speed = 5.0f;
    private:
        float m_pitch;
        float m_yaw;
        glm::vec3 m_velocity;
    };

    gl_camera default_camera = gl_camera(glm::vec3(0.0f, 0.0f, 30.0f));
    static bool first_mouse = true;
    static float yaw = 0.0f;
    static float pitch = 0.0f;
}

void default_framebuffer_size_callback(GLFWwindow *, int width, int height) {
    glViewport(0, 0, width, height);
    mk::default_camera.aspect = static_cast<float>(width) / height;
}

void default_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mk::first_mouse = true;
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        GLint state;
        glGetIntegerv(GL_POLYGON_MODE, &state);
        switch (state) {
        case GL_FILL:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        default:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        }
    }
}

void handle_input(GLFWwindow *window) {
    static float delta_time = 0.0f;
    static float last_frame = 0.0f;

    if (!mk::default_camera.movement_enabled) { return; }

    float current_frame = static_cast<float>(glfwGetTime());
    delta_time = current_frame - last_frame;
    last_frame = current_frame;

    float camera_speed = mk::default_camera.speed * delta_time;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        //mk::default_camera.pos += camera_speed * mk::default_camera.front;
        mk::default_camera.pos += camera_speed * mk::default_camera.get_velocity();
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        mk::default_camera.pos -= camera_speed * mk::default_camera.get_velocity();
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        mk::default_camera.pos -= glm::normalize(glm::cross(mk::default_camera.front, mk::default_camera.up)) * camera_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        mk::default_camera.pos += glm::normalize(glm::cross(mk::default_camera.front, mk::default_camera.up)) * camera_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        mk::default_camera.pos.y += camera_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        mk::default_camera.pos.y -= camera_speed;
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    static double   last_x = xpos;
    static double   last_y = ypos;
    static double   pitch = 0.0;
    static double   yaw = 0.0;
    if (mk::first_mouse) {
        last_x = xpos;
        last_y = ypos;
        //mk::first_mouse = false;
    }

    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL || !mk::default_camera.movement_enabled) return;

    double x_offset = xpos - last_x;
    double y_offset = last_y - ypos;
    last_x = xpos;
    last_y = ypos;

    double sensitivity = 0.1f;
    x_offset *= sensitivity;
    y_offset *= sensitivity;

    yaw += x_offset;
    pitch += y_offset;

    if (pitch > 89.0)
        pitch = 89.0;
    if (pitch < -89.0)
        pitch = -89.0;

    if (mk::first_mouse) {
        pitch = mk::default_camera.get_pitch();
        yaw = mk::default_camera.get_yaw();
        mk::first_mouse = false;
    }

    mk::default_camera.set_rotation(pitch, yaw);
    mk::yaw = yaw;
    mk::pitch = pitch;
}

namespace controls {
    static float offset = 0.0f;
}

void scroll_callback(GLFWwindow *, double, double yoffset) {
    controls::offset += yoffset;
}

struct window_init_options {
    int             p_width;
    int             p_height;
    const char  *   p_title;
    GLFWmonitor *   p_monitor;
    GLFWwindow  *   p_share;
};

class gl_context {
public:
    gl_context(window_init_options options) : m_window(nullptr, glfwDestroyWindow) {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glfw_version_major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glfw_version_minor);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 16);

        m_window.reset(glfwCreateWindow(
            options.p_width,
            options.p_height,
            options.p_title,
            options.p_monitor,
            options.p_share
        ));
        m_window_title = options.p_title;

        if (m_window == nullptr) {
            throw std::runtime_error("Failed to initialize GLFW window.");
        }
        glfwMakeContextCurrent(m_window.get());

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            throw std::runtime_error("Failed to initialize GLAD.");
        }

        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, options.p_width, options.p_height);
        glfwSetFramebufferSizeCallback(m_window.get(), default_framebuffer_size_callback);
        glfwSetKeyCallback(m_window.get(), default_key_callback);
        glfwSetCursorPosCallback(m_window.get(), mouse_callback);
        glfwSetScrollCallback(m_window.get(), scroll_callback);

        mk::default_camera.aspect = static_cast<float>(options.p_width) / options.p_height;

        // Initialize IMGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui_ImplGlfw_InitForOpenGL(m_window.get(), true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    ~gl_context() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwTerminate();
    }

    GLFWwindow *get_window() noexcept {
        return m_window.get();
    }

    std::string get_title() const noexcept {
        return m_window_title;
    }

    void set_title(std::string title) {
        m_window_title = title;
        glfwSetWindowTitle(m_window.get(), m_window_title.data());
    }

private:
    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window;
    std::string m_window_title;
};

namespace gl_constants {
    template <typename T> constexpr GLenum gl_enum = 0x0;
    template <> constexpr GLenum gl_enum<float_t> =     GL_FLOAT;
    template <> constexpr GLenum gl_enum<double_t> =    GL_DOUBLE;
    template <> constexpr GLenum gl_enum<int8_t> =      GL_BYTE;
    template <> constexpr GLenum gl_enum<int16_t> =     GL_SHORT;
    template <> constexpr GLenum gl_enum<int32_t> =     GL_INT;
    template <> constexpr GLenum gl_enum<uint8_t> =     GL_UNSIGNED_BYTE;
    template <> constexpr GLenum gl_enum<uint16_t> =    GL_UNSIGNED_SHORT;
    template <> constexpr GLenum gl_enum<uint32_t> =    GL_UNSIGNED_INT;
}

template <typename T>
concept GLType = requires(T) {
    { gl_constants::gl_enum<T> } -> std::convertible_to<GLenum>;
    { gl_constants::gl_enum<T> != 0x0 };
};

namespace mk {
    class location {
    public:
        location() : pos(0) { }
        location(glm::vec3 p_pos) : pos(p_pos) { }

        glm::mat4 get_matrix() const noexcept {
            return glm::translate(glm::identity<glm::mat4>(), pos);
        }

        glm::vec3 pos;
    };

    namespace geo {
        static constexpr size_t error_id = 0ULL;

        size_t next_id() {
            static auto __next_id = 1ULL;
            return __next_id++;
        }

        /* Initialized statically in implementation of mk::geo::geometry where geometry::set_vertices() is unsafe */
        class __warn_geometry_reinit { 
        public: __warn_geometry_reinit() { std::cout << "Warning: Changing vertices for this object is not recommended.\n"; } 
        };

        class geometry {
        public:
            virtual size_t get_id() const noexcept = 0;
            virtual GLuint get_vao() const noexcept = 0;
            virtual GLuint get_vbo() const noexcept = 0;
            virtual const mk::location &get_location() const noexcept = 0;
            virtual const std::vector<float> &get_vertices() const noexcept = 0;

            virtual mk::location &location() noexcept = 0;
            
            virtual void set_vertices(std::vector<float> vertices) = 0;

            virtual void draw() const = 0;
        };

        constexpr size_t TRIANGLE_VERTEX_COUNT = 3;

        class triangle : public geometry {
        private:
            void build() {
                glGenVertexArrays(1, &m_vao);
                glBindVertexArray(m_vao);
                glGenBuffers(1, &m_vbo);
                glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void *>(0));
                glEnableVertexAttribArray(0);

                m_id = next_id();
            }

            // for copy constructor
            triangle(std::vector<float> vertices, bool stupid) : m_vertices(std::move(vertices)) {
                build();
            }
        public:
            triangle(std::array<float, TRIANGLE_VERTEX_COUNT * 3> vertices) : m_location() {
                for (auto &&v : vertices) m_vertices.push_back(v);
                build();
            }

            ~triangle() {
                glDeleteVertexArrays(1, &m_vao);
                glDeleteBuffers(1, &m_vbo);
            }

            triangle(const triangle &other) : triangle(other.m_vertices, true) { }

            triangle &operator=(const triangle &other) {
                return *this = triangle(other);
            }

            triangle(triangle &&other) {
                std::swap(m_vao, other.m_vao);
                std::swap(m_vbo, other.m_vbo);
                std::swap(m_location, other.m_location);
                std::swap(m_vertices, other.m_vertices);
                std::cout << "move\n";
            }

            triangle &operator=(triangle &&other) {
                std::swap(m_vao, other.m_vao);
                std::swap(m_vbo, other.m_vbo);
                std::swap(m_location, other.m_location);
                std::swap(m_vertices, other.m_vertices);
                std::cout << "move\n";
                return *this;
            }

            size_t get_id() const noexcept override {
                return m_id;
            }

            GLuint get_vao() const noexcept override {
                return m_vao;
            }

            GLuint get_vbo() const noexcept override {
                return m_vbo;
            }

            const mk::location &get_location() const noexcept override {
                return m_location;
            }

            const std::vector<float> &get_vertices() const noexcept override {
                return m_vertices;
            }

            mk::location &location() noexcept override {
                return m_location;
            }

            void set_vertices(std::vector<float> vertices) override {
                m_vertices = std::move(vertices);
                glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof m_vertices, m_vertices.data(), GL_STATIC_DRAW);
            }

            void draw() const override {
                glBindVertexArray(m_vao);
                glDrawArrays(GL_TRIANGLES, 0, TRIANGLE_VERTEX_COUNT);
            }

        private:
            GLuint m_id;
            GLuint m_vao;
            GLuint m_vbo;
            mk::location m_location;
            std::vector<float> m_vertices;
        };

        constexpr float __cube_vertices[36 * 3] = {
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,

             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,

            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f
        };

        class cube : public geometry {
        public:
            cube() : m_location(glm::vec3(0.0f)) {
                glGenVertexArrays(1, &m_vao);
                glBindVertexArray(m_vao);
                glGenBuffers(1, &m_vbo);
                glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof __cube_vertices, __cube_vertices, GL_STATIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void *>(0));
                glEnableVertexAttribArray(0);

                m_vertices.assign(__cube_vertices, __cube_vertices + 36 * 3);
                m_id = next_id();
            }

            ~cube() {
                glDeleteVertexArrays(1, &m_vao);
                glDeleteBuffers(1, &m_vbo);
            }

            cube(const cube &other) {
                this->m_vertices = other.m_vertices;
                this->m_location = other.m_location;
                this->m_id = next_id();

                glGenVertexArrays(1, &m_vao);
                glBindVertexArray(m_vao);
                glGenBuffers(1, &m_vbo);
                glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof __cube_vertices, this->m_vertices.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void *>(0));
                glEnableVertexAttribArray(0);
            }

            cube &operator=(const cube &other) {
                return *this = cube(other);
            }

            cube(cube &&other) noexcept {
                std::swap(this->m_vertices, other.m_vertices);
                std::swap(this->m_location, other.m_location);
                std::swap(this->m_id, other.m_id);
                std::swap(this->m_vao, other.m_vao);
                std::swap(this->m_vbo, other.m_vbo);
            }

            cube &operator=(cube &&other) noexcept {
                return *this = cube(other);
            }

            std::size_t get_id() const noexcept override {
                return m_id;
            }

            GLuint get_vao() const noexcept override {
                return m_vao;
            }

            GLuint get_vbo() const noexcept override {
                return m_vbo;
            }

            const mk::location &get_location() const noexcept override {
                return m_location;
            }

            const std::vector<float> &get_vertices() const noexcept override {
                return m_vertices;
            }

            mk::location &location() noexcept override {
                return m_location;
            }

            void set_vertices(std::vector<float> vertices) override {
                static __warn_geometry_reinit _w{};
                m_vertices = std::move(vertices);
                glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                glBufferData(GL_ARRAY_BUFFER, 108 * sizeof(float), m_vertices.data(), GL_STATIC_DRAW);
            }

            void draw() const override {
                glBindVertexArray(m_vao);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

        private:
            std::size_t m_id;
            GLuint m_vao;
            GLuint m_vbo;
            mk::location m_location;
            std::vector<float> m_vertices;
        };

        std::shared_ptr<geometry> create_triangle(std::array<float, 9> vertices) {
            return std::shared_ptr<geometry>(new triangle(std::move(vertices)));
        }

        std::shared_ptr<geometry> create_cube() {
            return std::shared_ptr<geometry>(new cube());
        }
    }

    class light {
    public:
        light() : m_id{ geo::next_id() }, m_location{ } {
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);
            glGenBuffers(1, &m_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof geo::__cube_vertices, geo::__cube_vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void *>(0));
            glEnableVertexAttribArray(0);
        }

        // TODO copy and move constructors

        GLuint get_id() const noexcept { return m_id; }
        GLuint get_vao() const noexcept { return m_vao; }
        GLuint get_vbo() const noexcept { return m_vbo; }
        const mk::location &get_location() const noexcept { return m_location; }
        mk::location &location() noexcept { return m_location; }

        void draw() {
            glBindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

    private:
        std::size_t m_id;
        GLuint m_vao;
        GLuint m_vbo;
        mk::location m_location;
    };
}

namespace mk {
    class shader {
    public:
        static shader create_shader(const char *vertex_shader_src, const char *fragment_shader_src) {
            GLint success;
            constexpr auto info_log_size = 512;
            char info_log[info_log_size];

            GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex_shader, 1, &vertex_shader_src, nullptr);
            glCompileShader(vertex_shader);
            glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
            if (success == GL_FALSE) {
                glGetShaderInfoLog(vertex_shader, info_log_size, nullptr, info_log);
                std::cout << "Vertex shader could not be compiled:\n" << info_log << '\n';
            }

            GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment_shader, 1, &fragment_shader_src, nullptr);
            glCompileShader(fragment_shader);
            glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
            if (success == GL_FALSE) {
                glGetShaderInfoLog(fragment_shader, info_log_size, nullptr, info_log);
                std::cout << "Fragment shader could not be compiled:\n" << info_log << '\n';
            }

            GLuint shader_program = glCreateProgram();
            glAttachShader(shader_program, vertex_shader);
            glAttachShader(shader_program, fragment_shader);
            glLinkProgram(shader_program);
            glGetShaderiv(shader_program, GL_LINK_STATUS, &success);
            if (success == GL_FALSE) {
                glGetShaderInfoLog(shader_program, info_log_size, nullptr, info_log);
                std::cout << "Shader program linkage failure:\n" << info_log << '\n';
            }

            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);

            return shader(shader_program);
        }

        shader(GLuint shader_program_id) : m_shader_program_id(shader_program_id) { }

        GLuint get_program() const noexcept {
            return m_shader_program_id;
        }

    private:
        GLuint m_shader_program_id;
    };
}

class gl_scene {
public:
    gl_scene() : m_sky_color({}) {

    }

    void draw(mk::shader &draw_shader) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(draw_shader.get_program());
        GLint transform_loc = glGetUniformLocation(draw_shader.get_program(), "transform");

        auto projection = mk::default_camera.get_perspective();
        auto view = mk::default_camera.get_view();

        for (auto &&[key, shape] : geometries) {
            auto transform = projection * view * shape->location().get_matrix();
            glUniformMatrix4fv(transform_loc, 1, GL_FALSE, glm::value_ptr(transform));
            shape->draw();
        }
    }

    void set_sky_color(float r, float g, float b) {
        m_sky_color = { r, g, b, 1.0f };
        glClearColor(
            m_sky_color[0],
            m_sky_color[1],
            m_sky_color[2],
            m_sky_color[3]
        );
    }

    void set_sky_color(float r, float g, float b, float a) {
        m_sky_color = { r, g, b, a };
        glClearColor(
            m_sky_color[0],
            m_sky_color[1],
            m_sky_color[2],
            m_sky_color[3]
        );
    }

    const std::array<float, 4> &get_sky_color() const noexcept { return m_sky_color; }

    void add_geometry(std::shared_ptr<mk::geo::geometry> geometry) {
        geometries.insert({ geometry->get_id(), geometry });
    }

    std::shared_ptr<mk::geo::geometry> get_geometry(size_t key) {
        auto match = geometries.find(key);
        if (match == geometries.end()) {
            return match->second;
        }
        return nullptr;
    }

    std::unordered_map<std::size_t, std::shared_ptr<mk::geo::geometry>> geometries;
private:
    std::array<float, 4> m_sky_color;
};

//template <typename Func>
//void static_run(Func &&l) {
//    std::invoke(l);
//}

template <typename Func>
struct static_run {
    static_run(Func &&l) { std::invoke(l); }
};

void __Deprecated_RecreateSphere(
        float sphere_radius, float sector_count, float stack_count,
        GLuint &sphere_vao, GLuint &sphere_vbo, GLuint &sphere_ebo, 
        std::vector<float> &sphere_vertices,  std::vector<int> &sphere_indices) {
    static constexpr float PI = 3.14159265359;

    sphere_vertices.clear();
    sphere_indices.clear();
    glDeleteBuffers(1, &sphere_vbo);
    glDeleteBuffers(1, &sphere_ebo);
    glDeleteVertexArrays(1, &sphere_vao);

    float x, y, z, xy;
    float nx, ny, nz, length_inv = 1.0f / sphere_radius;
    float s, t;

    float sector_step = 2 * PI / sector_count;
    float stack_step = PI / stack_count;
    float sector_angle, stack_angle;

    for (int i = 0; i <= stack_count; ++i) {
        stack_angle = PI / 2 - i * stack_step;
        xy = sphere_radius * cosf(stack_angle);
        z = sphere_radius * sinf(stack_angle);

        for (int j = 0; j <= sector_count; ++j) {
            sector_angle = j * sector_step;

            // vertex position (x, y, z)
            x = xy * cosf(sector_angle);
            y = xy * sinf(sector_angle);
            sphere_vertices.push_back(x);
            sphere_vertices.push_back(y);
            sphere_vertices.push_back(z);

            // normalized vertex normal (nx, ny, nz)

            // vertex tex coord (s, t) range between [0, 1]
        }
    }

    int k1, k2;
    for (int i = 0; i < stack_count; ++i) {
        k1 = i * (sector_count + 1);
        k2 = k1 + sector_count + 1;

        for (int j = 0; j < sector_count; ++j, ++k1, ++k2) {
            if (i != 0) {
                sphere_indices.push_back(k1);
                sphere_indices.push_back(k2);
                sphere_indices.push_back(k1 + 1);
            }

            if (i != stack_count - 1) {
                sphere_indices.push_back(k1 + 1);
                sphere_indices.push_back(k2);
                sphere_indices.push_back(k2 + 1);
            }
        }
    }

    glGenVertexArrays(1, &sphere_vao);
    glBindVertexArray(sphere_vao);
    glGenBuffers(1, &sphere_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
    glBufferData(GL_ARRAY_BUFFER, sphere_vertices.size() * sizeof(float), sphere_vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void *>(0));
    glGenBuffers(1, &sphere_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere_indices.size() * sizeof(int), sphere_indices.data(), GL_STATIC_DRAW);
}

int main() {
    gl_context context({ 800, 600, "OpenGL Program" });
    gl_scene default_scene;

    auto triangle1 = mk::geo::create_triangle({ {
            0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 1.0f
        } });

    auto triangle2 = mk::geo::create_triangle({ {
            0.0f,  0.0f, 0.0f,
           -1.0f,  0.0f, 0.0f,
            0.0f, -1.0f, 1.0f
        } });

    auto cube1 = mk::geo::create_cube();
    auto cube2 = mk::geo::create_cube();
    cube2->location().pos = glm::vec3{ 10, 3, 0 };

    default_scene.add_geometry(triangle1);
    default_scene.add_geometry(triangle2);
    default_scene.add_geometry(cube1);
    default_scene.add_geometry(cube2);

    srand(time(nullptr));

    for (auto i = 0; i < 100; ++i) {
        auto cube = mk::geo::create_cube();
        cube->location().pos = glm::vec3{
            fmod(static_cast<double>(rand()) / RAND_MAX * 5, 5.0),
            fmod(static_cast<double>(rand()) / RAND_MAX * 5, 5.0),
            fmod(static_cast<double>(rand()) / RAND_MAX * 5, 5.0)
        };
        auto pos = glm::translate(glm::identity<glm::mat4>(), glm::vec3{ 5, 5, 5 }) * glm::vec4{ 1.0f };
        cube->location().pos += glm::vec3{ pos.x, pos.y, pos.z };
        default_scene.add_geometry(cube);
    }

    const char *glsl_vertex =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;"
        "out vec3 color;"
        ""
        "uniform mat4 transform;"
        ""
        "void main() {"
        "    gl_Position = transform * vec4(aPos, 1.0f);"
        "    color = vec3(0.3);"
        "}";

    const char *glsl_fragment =
        "#version 330 core\n"
        "out vec4 FragColor;"
        "in vec3 color;"
        ""
        "void main() {"
        "    FragColor = vec4(color, 1.0);"
        "}";

    mk::shader shader = mk::shader::create_shader(glsl_vertex, glsl_fragment);
    GLint transform_loc = glGetUniformLocation(shader.get_program(), "transform");

    // -- START OF LIGHTING

    const char *glsl_light_vertex =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;"
        ""
        "uniform mat4 transform;"
        ""
        "void main() {"
        "    gl_Position = transform * vec4(aPos, 1.0);"
        "}";

    const char *glsl_light_fragment =
        "#version 330 core\n"
        "out vec4 FragColor;"
        ""
        "uniform vec3 object_color;"
        "uniform vec3 light_color;"
        ""
        "void main() {"
        "    FragColor = vec4(light_color * object_color, 1.0);"
        "}";

    const char *glsl_light_fragment2 =
        "#version 330 core\n"
        "out vec4 FragColor;"
        ""
        "void main() {"
        "    FragColor = vec4(1.0);"
        "}";

    mk::shader light_shader = mk::shader::create_shader(glsl_light_vertex, glsl_light_fragment);
    GLint light_transform_loc = glGetUniformLocation(light_shader.get_program(), "transform");
    GLint object_color_loc = glGetUniformLocation(light_shader.get_program(), "object_color");
    GLint light_color_loc = glGetUniformLocation(light_shader.get_program(), "light_color");

    mk::shader light_object_shader = mk::shader::create_shader(glsl_light_vertex, glsl_light_fragment2);
    GLint light_object_transform_loc = glGetUniformLocation(light_object_shader.get_program(), "transform");

    auto light_source = mk::geo::create_cube();
    auto light_color = glm::vec3{ 0.33f, 0.42f, 0.18f };
    auto toy_color = glm::vec3{ 1.0f, 0.5f, 0.31f };
    auto result = light_color * toy_color;

    light_source->location().pos = glm::vec3{ 1.2f, 1.0f, 2.0f };
    cube1->location().pos = light_source->location().pos + glm::vec3{ 1.0, 0.0, 0.0 };

    // -- END OF LIGHTING

    // -- IMGUI INIT

    //ImGuiIO io = ImGui::GetIO(); (void)io;
    bool show_demo_window = true;

    // -- END OF IMGUI INIT

    std::vector<glm::vec3> grid_vertices;
    std::vector<glm::uvec4> grid_indices;

    int radius = 30;
    int slices = radius * 2;

    for (int j = 0; j <= slices; ++j) {
        for (int i = 0; i <= slices; ++i) {
            float x = static_cast<float>(i) / slices * slices;
            float y = 0/* static_cast<float>(i*i / (j+1)) + i*/;
            float z = static_cast<float>(j) / slices * slices;
            grid_vertices.push_back(glm::vec3{ x, y, z });
        }
    }

    for (int j = 0; j < slices; ++j) {
        for (int i = 0; i < slices; ++i) {
            int row1 = j        * (slices + 1);
            int row2 = (j + 1)  * (slices + 1);

            grid_indices.push_back(glm::uvec4{ row1 + i, row1 + i + 1, row1 + i + 1, row2 + i + 1 });
            grid_indices.push_back(glm::uvec4{ row2 + i + 1, row2 + i, row2 + i, row1 + i });
        }
    }

    GLuint grid_vao;
    GLuint grid_vbo;
    GLuint grid_ebo;
    glGenVertexArrays(1, &grid_vao);
    glBindVertexArray(grid_vao);

    glGenBuffers(1, &grid_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
    glBufferData(GL_ARRAY_BUFFER, grid_vertices.size() * sizeof(glm::vec3), glm::value_ptr(grid_vertices[0]), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, static_cast<void *>(0));

    glGenBuffers(1, &grid_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, grid_indices.size() * sizeof(glm::uvec4), glm::value_ptr(grid_indices[0]), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint grid_idx_length = static_cast<GLuint>(grid_indices.size() * 4);

    // origin axis

    const char *axis_vertex =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;"
        ""
        "uniform mat4 transform;"
        "out vec3 pos;"
        ""
        "void main() {"
        "    gl_Position = transform * vec4(aPos, 1.0);"
        "    pos = aPos;"
        "}";

    const char *axis_fragment =
        "#version 330 core\n"
        "out vec4 FragColor;"
        ""
        "in vec3 pos;"
        ""
        "void main() {"
        "    FragColor = vec4(ceil(pos), 1.0);"
        "}";

    mk::shader axis_shader = mk::shader::create_shader(axis_vertex, axis_fragment);
    GLint axis_transform_loc = glGetUniformLocation(axis_shader.get_program(), "transform");

    std::array<glm::vec3, 6> line_vertices{
        glm::vec3{ 0.0f, 0.0f, 0.0f },
        glm::vec3{ 2.0f, 0.0f, 0.0f },
        glm::vec3{ 0.0f, 0.0f, 0.0f },
        glm::vec3{ 0.0f, 2.0f, 0.0f },
        glm::vec3{ 0.0f, 0.0f, 0.0f },
        glm::vec3{ 0.0f, 0.0f, 2.0f }
    };

    GLuint origin_vao;
    GLuint origin_vbo;
    glGenVertexArrays(1, &origin_vao);
    glBindVertexArray(origin_vao);
    glGenBuffers(1, &origin_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, origin_vbo);
    glBufferData(GL_ARRAY_BUFFER, line_vertices.size() * sizeof(glm::vec3), glm::value_ptr(line_vertices[0]), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), static_cast<void *>(0));

    // end of origin axis

    // begin sphere

    constexpr float PI = 3.14159265359;
    float sphere_radius = 5;
    float sector_count = 5;
    float stack_count = 5;

    std::vector<float> sphere_vertices;
    std::vector<float> sphere_normals;
    std::vector<float> sphere_texcoords;

    float x, y, z, xy;
    float nx, ny, nz, length_inv = 1.0f / sphere_radius;
    float s, t;

    float sector_step = 2 * PI / sector_count;
    float stack_step = PI / stack_count;
    float sector_angle, stack_angle;

    for (int i = 0; i <= stack_count; ++i) {
        stack_angle = PI / 2 - i * stack_step;
        xy = sphere_radius * cosf(stack_angle);
        z = sphere_radius * sinf(stack_angle);

        for (int j = 0; j <= sector_count; ++j) {
            sector_angle = j * sector_step;

            // vertex position (x, y, z)
            x = xy * cosf(sector_angle);
            y = xy * sinf(sector_angle);
            sphere_vertices.push_back(x);
            sphere_vertices.push_back(y);
            sphere_vertices.push_back(z);

            // normalized vertex normal (nx, ny, nz)
            
            // vertex tex coord (s, t) range between [0, 1]
        }
    }

    std::vector<int> sphere_indices;
    std::vector<int> sphere_line_indices;

    int k1, k2;
    for (int i = 0; i < stack_count; ++i) {
        k1 = i * (sector_count + 1);
        k2 = k1 + sector_count + 1;

        for (int j = 0; j < sector_count; ++j, ++k1, ++k2) {
            if (i != 0) {
                sphere_indices.push_back(k1);
                sphere_indices.push_back(k2);
                sphere_indices.push_back(k1 + 1);
            }

            if (i != stack_count - 1) {
                sphere_indices.push_back(k1 + 1);
                sphere_indices.push_back(k2);
                sphere_indices.push_back(k2 + 1);
            }

            sphere_line_indices.push_back(k1);
            sphere_line_indices.push_back(k2);
            if (i != 0) {
                sphere_line_indices.push_back(k1);
                sphere_line_indices.push_back(k1 + 1);
            }
        }
    }

    GLuint sphere_vao;
    GLuint sphere_vbo;
    GLuint sphere_ebo;
    glGenVertexArrays(1, &sphere_vao);
    glBindVertexArray(sphere_vao);
    glGenBuffers(1, &sphere_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
    glBufferData(GL_ARRAY_BUFFER, sphere_vertices.size() * sizeof(float), sphere_vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void *>(0));
    glGenBuffers(1, &sphere_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere_indices.size() * sizeof(int), sphere_indices.data(), GL_STATIC_DRAW);
    std::cout << sphere_indices.size() << '\n';

    // end sphere

    mk::default_camera.pos.y = 0.0f;
    mk::default_camera.pos.z = 2.0f;
    mk::default_camera.set_rotation(0, 0);
    //mk::default_camera.field_of_view = 150;
    while (!glfwWindowShouldClose(context.get_window())) {
        handle_input(context.get_window());
        //default_scene.draw(shader);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto view = mk::default_camera.get_perspective() * mk::default_camera.get_view();

        // -- GRID
        glUseProgram(shader.get_program());
        glUniformMatrix4fv(transform_loc, 1, GL_FALSE, glm::value_ptr(glm::translate(view, glm::vec3{ -radius, 0, -radius })));
        glBindVertexArray(grid_vao);
        glDrawElements(GL_LINES, grid_idx_length, GL_UNSIGNED_INT, nullptr);

        // -- AXES
        glLineWidth(2);
        glUseProgram(axis_shader.get_program());
        glUniformMatrix4fv(axis_transform_loc, 1, GL_FALSE, glm::value_ptr(view));
        glBindVertexArray(origin_vao);
        glDrawArrays(GL_LINES, 0, 6);
        glLineWidth(1);

        // -- SCENE GEOMETRY (handled outside of default_scene to test lighting)
        glUseProgram(light_shader.get_program());
        glUniform3fv(object_color_loc, 1, glm::value_ptr(toy_color));
        glUniform3fv(light_color_loc, 1, glm::value_ptr(light_color));
        for (auto &&[_, shape] : default_scene.geometries) {
            auto transform = view * shape->get_location().get_matrix();
            glUniformMatrix4fv(light_transform_loc, 1, GL_FALSE, glm::value_ptr(transform));
            shape->draw();
        }

        static glm::vec3 sphere_pos{ 0, 0, 0 };
        auto sphere_transform = glm::translate(view, sphere_pos);
        sphere_transform = glm::rotate(sphere_transform, glm::radians(90.0f), glm::vec3{ 1.0f, 0.0f, 0.0f });
        sphere_transform = glm::rotate(sphere_transform, static_cast<float>(glfwGetTime()), glm::vec3{ 0.0f, 0.0f, 1.0f });
        glUniformMatrix4fv(light_transform_loc, 1, GL_FALSE, glm::value_ptr(sphere_transform));
        glBindVertexArray(sphere_vao);
        glDrawElements(GL_TRIANGLES, sphere_indices.size(), GL_UNSIGNED_INT, nullptr);

        // Temporarily disabled for debugging
        //glUseProgram(light_object_shader.get_program());
        //auto light_source_model = glm::scale(light_source->get_location().get_matrix(), glm::vec3{ 0.5, 0.5, 0.5 });
        //glUniformMatrix4fv(light_object_transform_loc, 1, GL_FALSE, glm::value_ptr(view * light_source_model));
        //light_source->draw();

        double c_x, c_y;
        glfwGetCursorPos(context.get_window(), &c_x, &c_y);
        int width, height;
        glfwGetWindowSize(context.get_window(), &width, &height);

        auto projection = glm::normalize(glm::unProject(
            glm::vec3{ c_x, static_cast<float>(height) - c_y, 1.0f},
            mk::default_camera.get_view(),
            mk::default_camera.get_perspective(),
            glm::vec4{ 0, 0, width, height }
        ) - mk::default_camera.pos);
        float distance = 0;
        (void)glm::intersectRayPlane(
            mk::default_camera.pos, 
            projection, 
            glm::vec3{ 0, controls::offset, 0 }, 
            glm::vec3{ 0, 1, 0 },
            distance
        );
        projection = distance * projection + mk::default_camera.pos;

        if (glfwGetInputMode(context.get_window(), GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
            auto model = glm::translate(glm::identity<glm::mat4>(), projection);
            model = glm::scale(model, glm::vec3{ 0.5f });
            auto transform 
                = mk::default_camera.get_perspective() 
                * mk::default_camera.get_view() 
                * model;
            glUseProgram(light_object_shader.get_program());
            glUniformMatrix4fv(light_object_transform_loc, 1, GL_FALSE, glm::value_ptr(transform));
            light_source->draw();
        }

        glfwSetWindowTitle(context.get_window(), 
            std::format("Coordinates: {}x, {}y, {}z",
                mk::default_camera.pos.x,
                mk::default_camera.pos.y,
                mk::default_camera.pos.z
            ).data());

        // -- IMGUI

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        //ImGui::ShowDemoWindow(&show_demo_window);

        static bool my_tool_active = true;

        if (my_tool_active) {
            ImGui::Begin("Camera Controls", &my_tool_active, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Close", "Ctrl+W")) { my_tool_active = false; }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::InputFloat("Camera speed", &mk::default_camera.speed);
            ImGui::InputFloat3("Position", glm::value_ptr(mk::default_camera.pos));
            
            static int anti_alias_samples = 1;
            static bool anti_aliasing = false;
            ImGui::Checkbox("Anti-Aliasing", &anti_aliasing);
            ImGui::SameLine();
            ImGui::SliderInt("Samples", &anti_alias_samples, 1, 8);
            if (anti_aliasing) {
                glfwWindowHint(GLFW_SAMPLES, anti_alias_samples);
                glEnable(GL_MULTISAMPLE);
            }
            else {
                glDisable(GL_MULTISAMPLE);
            }

            ImGui::End();
        }

        static bool __unused_condition = true;
        ImGui::Begin("Sphere Controls", &__unused_condition, ImGuiWindowFlags_MenuBar);
        static int param_radius = static_cast<int>(sphere_radius);
        static int param_sectors = static_cast<int>(sector_count);
        static int param_stacks = static_cast<int>(stack_count);

        ImGui::SliderInt("Radius", &param_radius, 0, 256);
        ImGui::SliderInt("Sectors", &param_sectors, 1, 64);
        ImGui::SliderInt("Stacks", &param_stacks, 1, 64);
        ImGui::SliderFloat3("Position", glm::value_ptr(sphere_pos), -50.0f, 50.0f, "%.3f", 1);

        if (param_radius != static_cast<int>(sphere_radius) 
            || param_sectors != static_cast<int>(sector_count) 
            || param_stacks != static_cast<int>(stack_count)) {
            sphere_radius = static_cast<float>(param_radius);
            sector_count = static_cast<float>(param_sectors);
            stack_count = static_cast<float>(param_stacks);

            __Deprecated_RecreateSphere(sphere_radius, sector_count, stack_count, sphere_vao, sphere_vbo, sphere_ebo, sphere_vertices, sphere_indices);
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(context.get_window(), &display_w, &display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // -- END OF IMGUI

        glfwSwapBuffers(context.get_window());
        glfwPollEvents();
    }
}
