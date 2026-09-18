# Optima Syntax

| Category | Type | Syntax Example / Description |
| :--- | :--- | :--- |
| **Primitive Types** | `i`, `b`, `f`, `str`, `c` | Integer, Boolean, Float, String, Char |
| **Derived Types** | `arr` | Fixed-size collections |
| **User-Defined** | `class` | **Standard:** Enforces literal intention/encapsulation by default <br>`class animal [...]` |
| | `open class` | **Struct-like:** Acts as a plain public data layout <br>`open class animal [...]` |
| | `enum` | Automatically numbers its internal declarations <br>`enum [a, b, c...]` |

> [NOTE]
> Blocks of code use brackets and **do not** end in semicolons.

## Declarations:
All declarations follow standard layout:
```text
let identifier: type block or built-in function, assigned value;
```

### Variables:
``` rust
    let x: i, 3;
```

### Collections:
* **Arrays:** Declared with their structural limits, types, and element values.
  ```rust
  let x: {arr(3), i}, \1, 2, 3\;
  ```
* **Maps:** Formatted as `map(key_type, value_type, size)`.
  ```rust
  let x: map(str, i, 2), \"first", "second"\, \1, 2\;
  ```

### Instantiation
The `?` symbol is utilized to indicate a new struct instance assignment.
```rust
let p: point, ?Point(2, 2);
```

---

## Loops and Conditionals:
> [NOTE]
> There are no `for` loops in the **O** programming language.

> [WARNING]
> Conditions **must** evaluate to strict boolean expressions. The compiler will reject any implicit integer or "truthy/falsy" values.

### If / Else:
```rust
if condition [
    // True logic execution
]
else [
    // Fallback logic execution
]
```
### While:
``` rust
while condition [
    ...
]
```

---

## Functions:
> [IMPORTANT]
> **Parameter Cap:** For optimization, functions are strictly limited to a **maximum of four parameters**.

### Creating functions:
The base type (e.g. `i`) precedes the function identifier

#### Out:
To return a value from a function, use the `out` keyword.

```rust
create i foo(type: parameter1, type: parameter2, type: parameter3, type: parameter4) [
    // Function body logic
    let v: i, 1;
    out v;
]
```


### Calling functions:
``` rust
call foo(argument1, argument2, argument3, argument4);
```

---

## Class:

> [NOTE]
> Closed classes (`class`) are not currently implemented yet.

### Structs:
``` rust
open class animal [...]
```

### Self-Reference:
The implicit keyword `inst` represents the current object, meaning methods can only accept a maximum of **3 parameters**.
It is equivalent to Python's `self` or C++'s `this`.

``` rust
open class Player [
    i: health;

    create i takeDamage(i: amount) [
        inst.health ~ inst.health - amount;
    ]
]
```

---

## Data Interaction:

### Reassignment:
Scale an existing value with `~`:
``` rust
let x: i, 0;
x~x+1;
```
Or reset the identifier absolutely using standard equalization:
``` rust
x = 10;
```

### Index:
Applies uniformly to both arrays and maps. Formatted as `identifier#index`.
``` rust
let x: {arr(3), i}, \1, 2, 3\;
let index: i, x#0;
```
### Element Push
Can be used for both arrays and maps. 
* **Arrays:** `push(identifier, value)`
* **Maps:** `push(identifier, key, value)`

> [NOTE]
> Arrays have a fixed declared size. You cannot push a value onto a full array.

``` rust
let array: {arr(3), i}, \1, 2\;
push(array, 3);

let newMap: map(i, i, 3), \1, 2, 3\, \10, 20, 30\;
push(newMap, 5, 50);
```

### Element Removal
This tool is native specifically to arrays. Formatted as `remove(identifier, index)`.
``` rust
let array: {arr(3), i}, \1, 2, 3\;
remove(array, 0);
```


### Collection Size:
The size operation can be performed on arrays and maps, formatted as: `#{identifier}`
``` rust
let array: {arr(3),i}, \1, 2, 3\;
let x: i, #{array};
```

### Reference:
Use a `@` symbol to indicate a reference instead of a copy.

``` rust
let x: i , 1;

let y: i@, x;
```

### Output Logging (Print):
Use a leading backtick `` ` `` to dump variable content to standard output.
``` rust
let x: i, 0;
`(x);
```

### Escape:
Use `$$` as the escape character and `$` for the literal dollar sign.
``` rust
let esc: c, '$$0';
```

---

## File I/O Modules

### Insert
Grants the current compilation context explicit code access to external files.
```rust
insert "example.ol"
```



