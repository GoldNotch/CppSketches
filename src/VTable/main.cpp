// Copyright 2024 JohnCorn
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     https://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <iostream>
#include <exception>
#include <memory>
#include <functional>

using namespace std;

struct Base
{
    Base(){ /* in the end of ctor */ vtable_ptr = &global_vtable_base; }

    /*virtual*/ ~Base()
    { 
        if (vtable_ptr->dtor)
            vtable_ptr->dtor(this);
        // unfortunatelly, there won't called inherited dtor even with that call
        cout << "Base::~Base()" << endl;
    }

    /*virtual*/ void Func()
    {
        if (!vtable_ptr->func) throw std::runtime_error("Pure Virtual Call"); 
        else vtable_ptr->func(this);
    }

private://------------ Base implementations -------------------
    static void Base_Func(void* _this){ cout << "Base::Func()" << endl; }

protected://----------------- Some declarations ------------------
    using func_ptr = void(*)(void* /*, Args... */);
    using dtor_ptr = void(*)(void*);
    struct vtable
    {
        dtor_ptr dtor = nullptr; // virtual destructor
        func_ptr func = nullptr; // virtual method
    };
    const vtable * vtable_ptr = nullptr; ///< object's vtable pointer
private:
    /// global vtable for Base
    static constexpr vtable global_vtable_base{nullptr, Base_Func};
};



struct Inherited : public Base
{
    Inherited() { /* in the end of ctor */ vtable_ptr = &global_vtable_inh; }

private:
    static void Inherited_Func(void * _this){ cout << "Inherited::Func()" << endl; }
    static void Inherited_Dtor(void * _this){ cout << "Inherited::Dtor()" << endl; }
private:
    static constexpr Base::vtable global_vtable_inh{Inherited_Dtor, Inherited_Func};
};



int main()
{
    Base* b = new Inherited();
    b->Func();
    delete b;
    return 0;
}