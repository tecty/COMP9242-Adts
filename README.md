# COMP9242-Adts

All the ADT you need while building your 9242 SOS project
The naming is violate the C naming schema since I want to have a OOP in C.
Also, it's quite nice to have layering and pretesting before moving the code into system.  

You can divide the `__` to function name and class name.

> DynamicArr__init  
> can be divided into:  
> DynamicArr.init()

And most of the function has first variable is self (in python), eg:

> void * DynamicArr__alloc(DynamicArr_t da, size_t * id)  
> can be translate into   
> da.alloc(size_t * id)  
> which da must have type DynamicArr_t

## Side Rules

`_s` sufix is for structure or the static structure (struct DynamicArr_s).   
`_t` is the pointer of that type (DynamicArr_t == struct struct DynamicArr_t).

## Side Note

The `AddressSpace` need to be imported in sos's root, other files could be import to your ADT's seL4 project.  
All the file are tested and applied into seL4. The tests are located in main of the file need to be commented before you import. 