#pragma once

struct BufferData {
	void* data{ nullptr };
	size_t size{ 0 };
    BufferData() = delete;
    BufferData(size_t size)
    {
        m_size = size;
        m_data = malloc(size);
    }
    ~BufferData()
    {
        if (m_data)
        {
            free(m_data);
        }
    }
    bool isValid() const { return m_data != nullptr; }
    const bool operator() const 
    {
        return isValid();
    }
};