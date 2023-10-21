#pragma once


/// @brief Helper class for witing SSI tag buffers
class BufferOutput
{
    public:
        BufferOutput(char *buffer, int length)
        : _buffer(buffer), _length(length), _written(0)
        {
        }

        uint16_t BytesWritten() { return _written; }

        void Append(const char *str)
        {
            auto len = strlen(str);
            if(len > _length)
                return;
            memcpy(_buffer, str, len);
            _buffer += len;
            _length -= len;
            _written += len;
        }

        void AppendEscaped(const char *str)
        {
            while(*str)
            {
                if(_length <= 0)
                    return;
                if(*str == '\"' || *str == '\\')
                {
                    *_buffer++ = '\\';
                    _length--;
                    _written++;
                    if(_length <= 0)
                        return;
                }
                *_buffer++ = *str++;
                _length--;
                _written++;
            }
        }


        void Append(const std::string &str)
        {
            auto len = str.length();
            if(len > _length)
                return;
            memcpy(_buffer, str.data(), str.length());
            _buffer += len;
            _length -= len;
            _written += len;

        }

        void Append(int value)
        {
            if(16 > _length)
                return;
            auto len = snprintf(_buffer, 16, "%d", value);
            _buffer += len;
            _length -= len;
            _written += len;
        }

        void AppendHex(uint32_t value)
        {
            if(_length < 9)
                return;
            sprintf(_buffer, "%08x", value);
            _buffer += 8;
            _length -= 8;
            _written += 8;
        }        

        void Append(char c)
        {
            if(!_length)
                return;
            *_buffer++ = c;
            _length --;
            _written ++;
        }


    private:
        char *_buffer;
        int _length;
        uint16_t _written;
};
