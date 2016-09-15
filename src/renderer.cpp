#include "renderer.hpp"

#include <vector>
#include <sstream>
#include <iostream>
#include <stdexcept>

void Renderer::setupWindow(int width, int height) {
  if (!glfwInit())
    throw std::runtime_error("Erro ao iniciar GLFW");
  
  m_window = glfwCreateWindow(width, height, "Pathtracer", NULL, NULL);
  if (!m_window) {
     glfwTerminate();
     throw std::runtime_error("Erro ao iniciar a janela GLFW");
  }

  glfwMakeContextCurrent(m_window);
  std::cout << "Versão GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
  if (glewInit() != GLEW_OK)
    throw std::runtime_error("Erro ao iniciar GLEW");

  glfwGetFramebufferSize(m_window, &m_width, &m_height);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

GLuint Renderer::compileShader(GLenum type, const std::string& shader) const {
  GLuint id = glCreateShader(type);
  const GLchar *buffer_ptr = shader.c_str();  
  glShaderSource(id, 1, &buffer_ptr, NULL);
  glCompileShader(id);
  
  GLint status;
  glGetShaderiv(id, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    GLint info_log_length;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
    GLchar* info_log = new GLchar[info_log_length + 1];
    glGetShaderInfoLog(id, info_log_length, NULL, info_log);

    std::stringstream ss;
    std::string typeName = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
    ss << "Falha de compilação no " << typeName << " shader:"
       << std::endl << info_log << std::endl;
    delete[] info_log; glDeleteShader(id);
    throw std::runtime_error(ss.str());
  }

  return id;
}

GLuint Renderer::linkShaders(GLuint vertex, GLuint fragment) const {
  GLuint program = glCreateProgram();

  glAttachShader(program, vertex);
  glAttachShader(program, fragment);
  glLinkProgram(program);

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    GLint info_log_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
    GLchar* info_log = new GLchar[info_log_length + 1];
    glGetProgramInfoLog(program, info_log_length, NULL, info_log);

    std::stringstream ss;
    ss << "Falha de ligação dos shaders:" << std::endl << info_log << std::endl;
    delete[] info_log; glDeleteProgram(program);
    throw std::runtime_error(ss.str());
  }

  return program;
}

void Renderer::setupFBO() {
  if (m_width <= 0 || m_height <= 0)
    throw std::runtime_error("O tamanho do frame buffer é inválido!");

  // Prepara a textura para o estimador de monte carlo.
  GLfloat *texture = new GLfloat[3 * m_width * m_height];
  for (GLint i = 0; i < 3 * m_width * m_height; ++i)
    texture[i] = 0.0f;
  
  GLuint mcTextureID;
  glGenTextures(1, &mcTextureID);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mcTextureID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, m_width, m_height, 0, GL_RGB, GL_FLOAT, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  delete[] texture;

  // Prepara o framebuffer
  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mcTextureID, 0);
  GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, drawBuffers);
  
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    throw std::runtime_error("ERRO INTERNO: Frame buffer está incompleto!");  
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setupProgram(const std::string& vertex, const std::string& fragment, const std::string& blit) {
  GLuint vertexID = compileShader(GL_VERTEX_SHADER, vertex);
  GLuint fragmentID = compileShader(GL_FRAGMENT_SHADER, fragment);
  GLuint blitID = compileShader(GL_FRAGMENT_SHADER, blit);

  m_blitProgram = linkShaders(vertexID, blitID);
  m_mainProgram = linkShaders(vertexID, fragmentID);
  glDeleteShader(vertexID); glDeleteShader(fragmentID); glDeleteShader(blitID);
  
  GLfloat vertices[16] = {-1.0,  1.0, 0.0, 1.0,
                          -1.0, -1.0, 0.0, 1.0,
                           1.0, -1.0, 0.0, 1.0,
                           1.0,  1.0, 0.0, 1.0};
  
  glGenBuffers(1, &m_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, static_cast<GLvoid*>(0));
  glEnableVertexAttribArray(0);
}

bool Renderer::scapeKey = false;
void keyboardCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
    Renderer::scapeKey = true;
}

void Renderer::render() {
  GLuint N = 0; // número de amostras calculadas.
  glfwSetKeyCallback(m_window, keyboardCallback);
  
  setupFBO();
  glViewport(0, 0, m_width, m_height);

  glUseProgram(m_blitProgram);
  GLint blitResLoc = glGetUniformLocation(m_blitProgram, "iResolution");
  GLint blitTexLoc = glGetUniformLocation(m_mainProgram, "iChannel");
  glUniform2f(blitResLoc, m_width, m_height);
  glUniform1i(blitTexLoc, 0);
  
  glUseProgram(m_mainProgram);
  GLint resLoc = glGetUniformLocation(m_mainProgram, "iResolution");
  GLint timeLoc = glGetUniformLocation(m_mainProgram, "time");
  GLint lightLoc = glGetUniformLocation(m_mainProgram, "nLights");
  GLint texLoc = glGetUniformLocation(m_mainProgram, "iChannel");
  GLint sampleNLoc = glGetUniformLocation(m_mainProgram, "sampleNumber");
  glUniform1i(lightLoc, m_lights);
  glUniform2f(resLoc, m_width, m_height);

  // 16 é o valor de MAX_ARRAY no template.glsl
  // se for alterar esse valor, altere-o no template também.
  // Eu sei que isso não é ótimo, mas preciso entregar o TP logo...
  for (GLint i = 0; i < 16; ++i)
    glUniform1i(texLoc+i, i);

  bool hasRendered = false;
  bool static_render = m_time >= 0.0;

  //std::cout << (glGetError() == GL_NONE) << std::endl;

  double initTime = glfwGetTime();
  glfwSwapInterval(1);
  while (!glfwWindowShouldClose(m_window)) {
    glfwPollEvents();
    if (Renderer::scapeKey && !hasRendered) {
      std::cout << "Amostras: " << N << std::endl
                << "Finalizado!" << std::endl
                << "Tempo: " << glfwGetTime() - initTime << std::endl;
      hasRendered = true;
    }
    if (hasRendered || Renderer::scapeKey) continue;

    
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glUseProgram(m_mainProgram);
    glUniform1f(timeLoc, static_render ? m_time : glfwGetTime());
    glUniform1ui(sampleNLoc, N++);
    glDrawArrays(GL_QUADS, 0, 4);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(m_blitProgram);
    glDrawArrays(GL_QUADS, 0, 4);
    
    glfwSwapBuffers(m_window);
    
    if (static_render)
      hasRendered =true;
  }
}

void Renderer::terminate() {
  glUseProgram(0);
  glDeleteProgram(m_mainProgram);
  glDeleteProgram(m_blitProgram);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &m_vbo); 
  glfwTerminate();
}
