inline Parser::Parser(const std::string& file) : m_file(file) {
  size_t loc = m_file.rfind("/");
  if (loc != std::string::npos)
    m_root_dir = m_file.substr(0, loc+1);
}