#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Renderer {
 public:
  Renderer() : m_time(-1) {};
  Renderer(float time) : m_time(time) {};
  void setupWindow(int width, int height);
  void setupProgram(const std::string& vertex, const std::string& fragment, const std::string& blit);
  void render();
  void terminate();
  void setLights(GLint lights) {m_lights = lights;}
  static bool scapeKey;
  
 private:
  void setupFBO();
  GLuint compileShader(GLenum type, const std::string& shader) const;
  GLuint linkShaders(GLuint vertex, GLuint fragment) const;
  
  GLFWwindow *m_window;      // janela da glfw
  GLuint m_mainProgram, m_blitProgram, m_vbo; // glProgram e array buffer
  GLuint m_fbo;              // frame buffer object
  GLint m_width, m_height;   // largura e altura da viewport
  GLint m_lights;            // quantidade de luzes na cena
  float m_time;              // tempo da simulacao para renderizacoes estaticas
};

#endif // RENDERER_HPP
