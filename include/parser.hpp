#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <tuple>

// Parser para ler o arquivo de entrada e gerar um shader
class Parser {
 public:
  Parser(const std::string& file);
  int getLights() {return m_lights;};
  std::vector<std::string> getTextures() {return m_textures;}
  std::string read();
  
 private:
  std::string readCamera(std::ifstream& input);
  std::string readLights(std::ifstream& input);
  std::string readMaterials(std::ifstream& input);
  std::string readProperties(std::ifstream& input);
  void readObjects(std::ifstream& input, std::string& objects, std::string& materialSelection, std::string& lights);
  void writeMaterial(std::stringstream& ss, int id1, int id2);
  
  std::string m_file, m_root_dir;
  int m_lights;
  std::unordered_map<int, std::tuple<float,float,float> > lightColor;
  std::unordered_map<int, bool> isLight;
  std::unordered_map<int,int> typeHash, colorHash, checkerHash, texHash;
  std::vector<std::string> m_textures;
  std::stringstream externalObjects;
};

// Classe simples que carrega um único shader em uma string
class ShaderReader {
 public:
  ShaderReader(const std::string& path) : m_path(path) {};
  std::string read();
  
 private:
  std::string m_path;
};

// Carregador de texturas
class TextureLoader {
public:
  void load(const std::string& path);
  void load(const std::vector<std::string>& textures);
  
private:  
  unsigned int count;
};

#include "parser.inl"

#endif // PARSER_HPP
