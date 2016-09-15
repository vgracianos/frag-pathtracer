#include "parser.hpp"
#include "renderer.hpp"

#include <fstream>
#include <sstream>
#include <streambuf>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

// Não verifica por erros no arquivo de cena...
std::string Parser::read() {
  std::ifstream input(m_file.c_str());

  if (!input.is_open())
    throw std::runtime_error("Arquivo não encontrado: " + m_file);

  // LEITURA DO ARQUIVO DE ENTRADA
  std::string camera = readCamera(input);
  std::string lights = readLights(input);
  std::string materials = readMaterials(input);
  std::string properties = readProperties(input);
  
  std::string objects, select;
  readObjects(input, objects, select, lights);

  // MONTAGEM DAS FUNCOES DO SHADER
  std::stringstream map;
  map << "float map(vec3 p) {" << std::endl
      << "float d = FAR;" << std::endl
      << objects << std::endl << "return d;"
      << std::endl << "}" << std::endl;

  std::stringstream selectMaterial;
  selectMaterial << "ivec3 selectMaterial(vec3 p) {" << std::endl
                 << "float d = FAR, aux; ivec3 mat = ivec3(0);"
                 << std::endl << select << std::endl
                 << "return mat;" << std::endl << "}" << std::endl;

  std::stringstream build;
  build << "void buildMaterialsAndLights() {" << std::endl
        << materials << properties << lights << std::endl
        << "}" << std::endl;
  return camera + externalObjects.str() + map.str() +
    selectMaterial.str() + build.str();
}

std::string Parser::readCamera(std::ifstream& input)
{
  std::string camx, camy, camz;
  std::string projx, projy, projz;
  std::string upx, upy, upz;
  std::string fov;
  std::stringstream camera;
  
  input >> camx >> camy >> camz
        >> projx >> projy >> projz
        >> upx >> upy >> upz >> fov;
  
  camera << "void buildCamera(out vec3 ro, out vec3 rd) {" << std::endl
         << "ro = vec3(" << camx << "," << camy << "," << camz << ");"
        //<< "ro += 0.05*(2.0*vec3(rand(), rand(), rand()) - 1.0);" // hacky DOF
         << std::endl << "vec3 t = vec3(" << projx << "," << projy
         << "," << projz << ")," << "n = vec3(" << upx << "," << upy
         << "," << upz << ");" << std::endl
         << "vec3 f = normalize(ro - t);" << std::endl
         << "vec3 r = normalize(cross(normalize(n), f));" << std::endl
         << "vec3 u = normalize(cross(f, r));" << std::endl
         << "vec2 uv = (-iResolution.xy+2.0*gl_FragCoord.xy)/iResolution.y;" << std::endl
         << "uv += 0.0055*(2.0*vec2(rand(), rand()) - 1.0);" << std::endl
         << "uv *= tan(0.5*" << fov << "*3.141592/180);" << std::endl
         << "rd = normalize(mat3(r,u,f)*vec3(uv, -1.0));" << std::endl
         << std::endl << "}" << std::endl;
  return camera.str();
}

std::string Parser::readLights(std::ifstream& input) {
  int nLights;
  std::stringstream lights;

  input >> nLights; m_lights = nLights;
  for (int i = 0; i < nLights; ++i) {
    std::string px, py, pz;
    std::string r, g, b;

    input >> px >> py >> pz >> r >> g >> b;
    lights << "lights["<<i<<"] = Light(0, vec3(" << px << "," << py << ","
           << pz << "),vec3(" << r << "," << g << "," << b << "),0);"
           << std::endl;
  }
  return lights.str();
}

std::string Parser::readMaterials(std::ifstream& input) {
  int nMaterials;
  std::stringstream materials;

  int colIndex = 0, checkIndex = 0, texIndex = 2;
  input >> nMaterials;
  for (int i = 0; i < nMaterials; ++i) {
    std::string type;

    input >> type;
    if (type == "solid") {
      std::string r, g, b;
      typeHash[i] = 0;
      colorHash[i] = colIndex++;
      input >> r >> g >> b;
      materials << "solidColors["<< colorHash[i] << "] = vec3("
                << r << "," << g << "," << b << ");" << std::endl;
      
    } else if (type == "checker") {
      std::string r[2], g[2], b[2], size;
      typeHash[i] = 1;
      checkerHash[i] = checkIndex++;
      input >> r[0] >> g[0] >> b[0] >> r[1] >> g[1] >> b[1] >> size;
      materials << "checkerColorA["<< checkerHash[i] << "] = vec3("
                << r[0] << "," << g[0] << "," << b[0] << ");" << std::endl;
      materials << "checkerColorB["<< checkerHash[i] << "] = vec3("
                << r[1] << "," << g[1] << "," << b[1] << ");" << std::endl;
      materials << "checkerSize["<< checkerHash[i] << "] = " << size << ";" << std::endl;
      
    } else if (type == "texmap") {
      std::string name, scale;
      typeHash[i] = 2;
      texHash[i] = texIndex++;
      input >> name >> scale;
      materials << "textureScale[" << texHash[i] << "] = " << scale << ";" << std::endl;
      m_textures.push_back(m_root_dir + name);
    }
      
  }
  return materials.str();
}

std::string Parser::readProperties(std::ifstream& input) {
  int nProperties;
  std::stringstream properties;

  input >> nProperties;
  for (int i = 0; i < nProperties; ++i) {
    float er, eg, eb, alpha;
    float kr, kt, ior;
    input >> er >> eg >> eb >> alpha >> kr >> kt >> ior;
    properties << "properties[" << i << "] = Properties(vec3("
               << er << "," << eg << "," << eb << ")," << alpha << ","
               << kr << "," << kt << "," << ior << ");" << std::endl;
    if (er > 0 || eg > 0 || eb > 0) {
      isLight[i] = true;
      lightColor[i] = std::make_tuple(er, eg, eb);
    }
  }

  return properties.str();
}

void Parser::writeMaterial(std::stringstream& ss, int id1, int id2) {
  ss << "if (aux < d) {d = aux; mat = ivec3(" << typeHash[id1];
  switch(typeHash[id1]) {
    case 0:
      ss << "," << colorHash[id1]; break;
    case 1:
      ss << "," << checkerHash[id1]; break;
    case 2:
      ss << "," << texHash[id1]; break;
  }
  ss << "," << id2 << ");}" << std::endl;
}

void Parser::readObjects(std::ifstream& input, std::string& objects, std::string& materialSelection, std::string& lights) {
  int nObjects;
  std::stringstream map, select, lightStream;
  
  input >> nObjects;
  for (int i = 0; i < nObjects; ++i) {
    int property, material;
    std::string type;
  
    input >> material >> property >> type;
    if (isLight[property] && type != "sphere" )
      throw std::runtime_error("Apenas esferas podem ser emissivas!");
    
    if (type == "sphere") {
      std::string x, y, z, r;
      input  >> x >> y >> z >> r;
      map << "d = min(d, sphere(p,vec4(" << x << "," << y << "," << z
          << "," << r << ")));" << std::endl;
      select << "aux = sphere(p,vec4(" << x << "," << y << "," << z
             << "," << r << "));" << std::endl;
      if (isLight[property]) {
        auto col = lightColor[property];
        lightStream << "lights["<<m_lights++<<"] = Light(1, vec3(" << x << "," << y << ","
                    << z << "),vec3(" << std::get<0>(col) << "," << std::get<1>(col) << ","
                    << std::get<2>(col) << ")," << r <<");" << std::endl;
      }
    } else if (type == "polyhedron") {
      int faces;
      input >> faces;
      if (faces <= 0)
        throw std::runtime_error("Poliedro sem faces!");
      map << "d = min(d,"; select << "aux = ";
      for (int j = 0; j+1 < faces; ++j) {
        std::string x, y, z, w;
        input >> x >> y >> z >> w;
        map << "min(plane(p,vec4("<<x<<","<<y<<","<<z<<","<<w<<")),";
        select << "min(plane(p,vec4("<<x<<","<<y<<","<<z<<","<<w<<")),";
      }
      std::string x, y, z, w;
      input >> x >> y >> z >> w;
      map << "plane(p,vec4("<<x<<","<<y<<","<<z<<","<<w<<")))";
      select << "plane(p,vec4("<<x<<","<<y<<","<<z<<","<<w<<"))";
      for (int j = 0; j+1 < faces; ++j) {
        map << ")"; select << ")";
      }
      map << ";" << std::endl;
      select << ";" << std::endl;
    
    } else if (type == "box") {
      std::string x, y, z, sx, sy, sz;
      input >> x >> y >> z >> sx >> sy >> sz;
      map << "d = min(d, box(p,vec3(" << x << "," << y << "," << z
          << "), vec3(" << sx << "," << sy << "," << sz << ")));" << std::endl;
      select << "aux = box(p,vec3(" << x << "," << y << "," << z
             << "), vec3(" << sx << "," << sy << "," << sz << "));" << std::endl;
      
    } else if (type == "torus") {
      std::string x, y, z, r1, r2;
      input >> x >> y >> z >> r1 >> r2;
      map << "d = min(d, torus(p,vec3(" << x << "," << y << "," << z
          << "), vec2(" << r1 << "," << r2 << ")));" << std::endl;
      select << "aux = torus(p,vec3(" << x << "," << y << "," << z
             << "), vec2(" << r1 << "," << r2 << "));" << std::endl;
 
    } else if (type == "cone") {
      std::string x, y, z, sx, sy, sz;
      input >> x >> y >> z >> sx >> sy;
      map << "d = min(d, cone(p,vec3(" << x << "," << y << "," << z
          << "), vec2(" << sx << "," << sy << ")));" << std::endl;
      select << "aux = cone(p,vec3(" << x << "," << y << "," << z
             << "), vec2(" << sx << "," << sy << "));" << std::endl;
      
    } else if (type == "cylinder") {
      std::string x, y, z, r1, r2;
      input >> x >> y >> z >> r1 >> r2;
      map << "d = min(d, cylinder(p,vec3(" << x << "," << y << "," << z
          << "), vec2(" << r1 << "," << r2 << ")));" << std::endl;
      select << "aux = cylinder(p,vec3(" << x << "," << y << "," << z
             << "), vec2(" << r1 << "," << r2 << "));" << std::endl;
    } else if (type == "disk") {
      float x, y, z, nx, ny, nz, r;
      input >> x >> y >> z >> nx >> ny >> nz >> r;
      float div = sqrt(nx * nx + ny*ny + nz*nz);
      nx /= div; ny /= div; nz /= div;
      map << "d = min(d, disk(p,vec3(" << x << "," << y << "," << z
          << "), vec3(" << nx << "," << ny << "," << nz <<")," << r << "));" << std::endl;
      select << "aux = disk(p,vec3(" << x << "," << y << "," << z
             << "), vec3(" << nx << "," << ny << "," << nz <<")," << r << ");" << std::endl;
      /*if (isLight[property]) {
        auto col = lightColor[property];
        lightStream << "lights["<<m_lights++<<"] = Light(1, vec3(" << x << "," << y << ","
                    << z << "),vec3(" << std::get<0>(col) << "," << std::get<1>(col) << ","
                    << std::get<2>(col) << ")," << "vec4("<<nx<<","<<ny<<","<<nz<<","<<r
                    <<"));" << std::endl;
                    }*/
      
    } else { // unknown type (try to load a shader for it)
      std::string x, y, z;
      ShaderReader reader("shaders/" + type + ".glsl");
      input >> x >> y >> z;
      map << "d = min(d, "+type+"(p,vec3(" << x << "," << y << "," << z
          << ")));" << std::endl;
      select << "aux = "+type+"(p,vec3(" << x << "," << y << "," << z
          << "));" << std::endl;
      externalObjects << reader.read();
    }
    writeMaterial(select, material, property);
  }

  objects = map.str();
  materialSelection = select.str();
  lights += lightStream.str();
}

// ====================== SHADER READER ======================
std::string ShaderReader::read() {
  std::string shader;
  std::ifstream input(m_path.c_str());
  
  if (!input.is_open())
    throw std::runtime_error("Arquivo não encontrado: " + m_path);

  input.seekg(0, std::ios::end);
  shader.reserve(input.tellg());
  input.seekg(0, std::ios::beg);
  
  shader.assign(std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>());
  
  return shader;
}

// ====================== PPM READER ======================
void TextureLoader::load(const std::string& path) {
  char buffer[256];
  unsigned char* image;
  unsigned int width, height;

  FILE *fp;
  fp = fopen(path.c_str(), "rb");
  
  try {
    if (!fp)
      throw std::runtime_error("Arquivo não encontrado: " + path);

    // Magic number
    if (!fgets(buffer, sizeof(buffer), fp))
      throw std::runtime_error("Erro durante a leitura do arquivo " + path);
    
    if (buffer[0] != 'P' || buffer[1] != '6')
      throw std::runtime_error(path + "não é um arquivo PPM válido (P6)");

    // Ignora os comentarios
    char c = getc(fp);
    while (c == '#') {
      while (getc(fp) != '\n');
      c = getc(fp);
    }
    ungetc(c, fp);

    // tamanho da imagem
    if (fscanf(fp, "%u %u", &width, &height) != 2)
      throw std::runtime_error("Erro durante a leitura do arquivo " + path);

    // profundidade de cor
    unsigned int depth;
    if (fscanf(fp, "%u", &depth) != 1)
      throw std::runtime_error("Erro durante a leitura do arquivo " + path);
    if (depth != 255)
      throw std::runtime_error("Profundidade de cor é diferente de 8 bpp no arquivo " + path);
    while (fgetc(fp) != '\n');

    image = new unsigned char[3*width*height];
    if (fread(image, 3*width, height, fp) != height)
      throw std::runtime_error("Erro durante a leitura do arquivo " + path);
    fclose(fp);
    
  } catch (std::runtime_error& e) {
    if (fp)
      fclose(fp);
    throw e;
  }

  // Carrega a textura no OpenGL.
  GLuint tex;
  glGenTextures(1, &tex);
  glActiveTexture(GL_TEXTURE0 + this->count);
  glBindTexture(GL_TEXTURE_2D, tex);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  delete[] image;
}

void TextureLoader::load(const std::vector<std::string>& textures) {
  this->count = 2;
  for (auto it = textures.begin(); it != textures.end(); ++it) {
    load(*it);
    this->count++;
  }
}
