# hadrJson
JSON library in ANSI C

# introduction
- support [json standard](https://www.json.org/json-en.html)
- developed by C89
- recursive descent parser
- only support utf-8 json document
- use dynamic array to store array element and object member

# feature
- parse (done)
    - null
    - true
    - false
    - number
    - string
    - array
    - object
- stringify (to be done)
- access (to be done)
- roundtrip speed test (to be done)

# run
## build test
~~~bash
make
~~~

## run unit test
~~~bash
./test
~~~

# FAQ
- Why the project named "hadrjon" ?
    - The "hadr" is from "hadron" in the Standard Model of particle physics. 
    - Not only for the unique naming, but also in memory of the [json-tutorial](https://github.com/miloyip/json-tutorial).
- Why create this project ?
    - Pratice my C programming skill.
    - Prepare for an in-depth study of compilation principles.

# thanks
unit test frame from [leptjson](https://github.com/miloyip/json-tutorial/blob/master/tutorial08/test.c)