// © 2021 NVIDIA Corporation

TextureVal::~TextureVal() {
    if (m_Memory)
        m_Memory->Unbind(*this);
}
