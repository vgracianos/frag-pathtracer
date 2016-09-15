#include "renderer.hpp"
#include "parser.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main(int argc, char *argv[])
{
  float time = -1;
  
  // Linux/Mac only...
  if (argc <= 1) {
    std::cout << argv[0] << " [entrada] [largura] [altura] [tempo]" << std::endl
              << "Os parâmetros [largura], [altura] e [tempo] são opcionais." << std::endl << std::endl
              << "ATENÇÃO: a sintaxe original dos arquivos de entrada foi alterada!!!" 
              << std::endl << "Utilize os arquivos no diretório scenes como entrada!!!" << std::endl;
    return EXIT_SUCCESS;
  }

  int width = 640, height = 480;
  if (argc >= 4) {
    width = atoi(argv[2]);
    height = atoi(argv[3]);
  }
  
  if (argc >= 5) {
    time = atof(argv[4]);
    if (time < 0) {
      std::cout << "Tempo precisa ser positivo!" << std::endl;
      return EXIT_FAILURE;
    } 
  }

  Renderer renderer(time);
  try {
    renderer.setupWindow(width, height);

    Parser parser(argv[1]);
    ShaderReader blitReader("shaders/blit.glsl");
    ShaderReader vertexReader("shaders/vertex.glsl");
    ShaderReader templateReader("shaders/template.glsl");

    std::string blitShader = blitReader.read();
    std::string vertexShader = vertexReader.read();
    std::string raytracerShader = templateReader.read() + parser.read();

    renderer.setLights(parser.getLights());
    renderer.setupProgram(vertexShader, raytracerShader, blitShader);
    
    TextureLoader texLoader;
    texLoader.load(parser.getTextures());    
    
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    renderer.terminate();
    return EXIT_FAILURE;
  }

  // Verifica por erros na OpenGL
  GLenum errorCode = glGetError();
  if (errorCode != GL_NO_ERROR) {
    std::cerr << "Erro no OpenGL! (Código " << errorCode << ")" << std::endl;
    renderer.terminate();
    return EXIT_FAILURE;
  }
  
  renderer.render();
  renderer.terminate();
  return EXIT_SUCCESS;
}
