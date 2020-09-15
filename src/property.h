#ifndef PROPERTY_H
#define PROPERTY_H

#include <functional>
#include <iostream>
#include <string>

template<class T>
class Property
{
    T _value;
    std::function<T (void)> _get;
    std::function<void(const T&)> _set;
public:
    Property()
    { }

    Property(
        std::function<T (void)> get,
        std::function<void(const T&)> set)
        : _get(get),
          _set(set)
    { }
    
    Property(
        std::function<T (void)> get)
        : _get(get)
    { }
    
    operator T () const
    {
        if (_get)
        {
            return _get();
        }
        
        return _value;
    }
    
    void operator = (const T& value)
    {
        if (_set)
        {
            _set(value);
        }
        else
        {
            _value = value;
        }
    }
};

#endif // PROPERTY_H
