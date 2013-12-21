/*
    Copyright 2013 Christopher Stones ( chris.stones@zoho.com )

    This file is part of IoCC.

    IoCC is free software: you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    IoCC is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty
	of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
	See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with Foobar. If not, see http://www.gnu.org/licenses/.
*/


#include <iostream>

#include <typeinfo>
#include <typeindex>
#include <map>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <exception>
#include <stdexcept>
#include <cassert>
#include <stdio.h>

namespace ioc_detail {

/**************************************************************************
 * Internal std::shared_ptr<T> wrapper.
 * 	Allows unrelated interfaces to be stored in the same keyed container.
 **************************************************************************/
class ISharedPtrWapper {
protected:
    virtual void * member__() = 0;
public:
    virtual ~ISharedPtrWapper() {}
    template<typename T> std::shared_ptr<T> &GetShared() {
        return *reinterpret_cast<std::shared_ptr<T>*>(member__());
    }
};
template<typename T>
class SharedPtrWapper
    : public ISharedPtrWapper
{
    std::shared_ptr<T> p;
protected:
    void * member__() {
        return &p;
    }
public:
    SharedPtrWapper(std::shared_ptr<T> p) : p(p) {}
    virtual ~SharedPtrWapper() {}
};

/**************************************************************************
 * Internal std::function<R(Args...)> wrapper.
 * 	Allows unrelated functions to be stored in the same keyed container.
 **************************************************************************/
class IInstantiatorWrapper {
protected:
    virtual void * member__() = 0;
public:
    virtual ~IInstantiatorWrapper() {}
    template<typename T> T &GetInstantiator() {
        return *reinterpret_cast<T*>(member__());
    }
};
template<typename T>
class InstantiatorWrapper
    :	public IInstantiatorWrapper
{
    T i;
protected:
    void * member__() {
        return &i;
    }
public:
    virtual ~InstantiatorWrapper() {}
    InstantiatorWrapper(T & i) : i(i) {}
};
/**************************************************************************/
}

namespace ioc {

/*** Inversion of control continer ***/
class IoCC {

    typedef std::type_index UnnamedTypeKey;
    typedef std::pair<std::type_index,std::string> NamedTypeKey;

    typedef std::map<UnnamedTypeKey, ioc_detail::ISharedPtrWapper*> UnnamedInstanceMap;
    typedef std::map<  NamedTypeKey, ioc_detail::ISharedPtrWapper*> NamedInstanceMap;

    typedef std::map<UnnamedTypeKey, ioc_detail::IInstantiatorWrapper*> UnnamedInstantiatorMap;
    typedef std::map<  NamedTypeKey, ioc_detail::IInstantiatorWrapper*> NamedInstantiatorMap;

    UnnamedInstanceMap unnamedInstanceMap;
    NamedInstanceMap   namedInstanceMap;

    UnnamedInstantiatorMap unnamedInstantiatorMap;
    NamedInstantiatorMap   namedInstantiatorMap;

public:

    class ResolverException : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class InstantiatorException : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

private:

    std::string ToString(const NamedTypeKey &namedTypeKey) {

        return std::string(namedTypeKey.first.name()) + " \"" + namedTypeKey.second + " \"";
    }

    std::string ToString(const UnnamedTypeKey &unnamedTypeKey) {

        return unnamedTypeKey.name();
    }

    // Store a wrapped inerface or instantiator in a named or unnamed keyed map.
    template<typename WrapperType, typename Map, typename T, typename Key>
    void Register__( Map & map, const Key & key, WrapperType * wrapper ) {

        typename Map::iterator itor = map.find(key);
        if(itor != map.end()) {
            delete itor->second;
            if(wrapper)
                itor->second = wrapper;
            else
                map.erase(itor);
        }
        else if(wrapper)
            map[key] = wrapper;
    }

    // Store a wrapped instance in a named or unnamed keyed map.
    template<typename WrapperType, typename Map, typename T, typename Key>
    void RegisterInstance__( Map & map, const Key & key, T & instance ) {

        WrapperType * wrapper =
            instance ? (new WrapperType(instance) ) : nullptr;

        Register__<WrapperType,Map,T,Key>(map, key, wrapper);
    }

    // Store a wrapped instantiator in a named or unnamed keyed map.
    template<typename WrapperType, typename Map, typename T, typename Key>
    void RegisterInstantiator__( Map & map, const Key & key, T & instance ) {

        WrapperType * wrapper = new WrapperType(instance);

        Register__<WrapperType,Map,T,Key>(map, key, wrapper);
    }

    template<typename T, typename Map, typename ...Args>
    std::shared_ptr<T> Instantiate(Map & map, const typename Map::key_type & key, Args... args) {

        typedef std::function<std::shared_ptr<T>(Args...)> InstantiatorType;

        typename Map::iterator itor =
            map.find(key);

        if( itor != map.end() ) {
            ioc_detail::IInstantiatorWrapper * instantiatorWrapper =
                itor->second;

            return instantiatorWrapper->GetInstantiator<InstantiatorType>()(args...);
        }

        throw InstantiatorException(ToString(key));
    }

    template<typename T, typename Map>
    std::shared_ptr<T> &ResolveInstance( Map & map, const typename Map::key_type & key ) {
        typename Map::iterator itor = map.find(key);
        if(itor != map.end()) {
            ioc_detail::ISharedPtrWapper * sharedWrapper = itor->second;
            return sharedWrapper->GetShared<T>();
        }

        throw ResolverException( ToString(key) );
    }

public:

    virtual ~IoCC() {
        for( auto i : unnamedInstanceMap     ) delete i.second;
        for( auto i :   namedInstanceMap     ) delete i.second;
        for( auto i : unnamedInstantiatorMap ) delete i.second;
        for( auto i :   namedInstantiatorMap ) delete i.second;
    }

    template<typename T>
    void Store(std::shared_ptr<T> instance) {
        RegisterInstance__<ioc_detail::SharedPtrWapper<T> >(unnamedInstanceMap, typeid(T), instance);
    }

    template<typename T>
    void Store(const std::string & name, std::shared_ptr<T> instance) {

        auto key = NamedTypeKey(typeid(T), name );
        RegisterInstance__<ioc_detail::SharedPtrWapper<T> >(namedInstanceMap, key, instance);
    }

    template<typename T>
    std::shared_ptr<T> &Retrieve( ) {
        return ResolveInstance<T>( unnamedInstanceMap, typeid(T) );
    }

    template<typename T>
    std::shared_ptr<T> &Retrieve( const std::string & name ) {

        auto key = NamedTypeKey(typeid(T), name );
        return ResolveInstance<T>(namedInstanceMap, key);
    }

    template<typename _Signature, typename Instantiator>
    void RegisterInstantiator(const std::string &name, Instantiator instantiator__) {

        std::function<_Signature> instantiator = instantiator__;

        auto key = std::pair<std::type_index,std::string>(typeid(instantiator), name );
        RegisterInstantiator__<ioc_detail::InstantiatorWrapper<decltype(instantiator)>>
                (namedInstantiatorMap, key, instantiator);
    }

    template<typename _Signature, typename Instantiator>
    void RegisterInstantiator(Instantiator instantiator__) {

        std::function<_Signature> instantiator = instantiator__;

        std::type_index key = typeid(instantiator);
        RegisterInstantiator__<ioc_detail::InstantiatorWrapper<decltype(instantiator)>>
                (unnamedInstantiatorMap, key, instantiator);
    }

    template<typename T, typename ...Args>
    std::shared_ptr<T> New(Args... args) {

        typedef std::function<std::shared_ptr<T>(Args...)> InstantiatorType;

        UnnamedTypeKey key = typeid(InstantiatorType);

        return Instantiate<T>(unnamedInstantiatorMap, key, args...);
    }

    template<typename T, typename ...Args>
    std::shared_ptr<T> NewNamed(const std::string & name, Args... args) {

        typedef std::function<std::shared_ptr<T>(Args...)> InstantiatorType;

        NamedTypeKey key = NamedTypeKey(typeid(InstantiatorType), name);

        return Instantiate<T>(namedInstantiatorMap, key, args...);
    }
};
}
