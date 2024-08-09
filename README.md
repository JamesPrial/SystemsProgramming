# SystemsProgramming
Projects produced for Systems Programming course CS214\n
CS214 - Code Quality Guidelines

Every function must have a comment describing the intended purpose and use of the function. Specifically, what the function is "for", not necessarily what it "does".

Additionally, you must document any assumptions the function makes, or constraints on its inputs. For example, "the pointer must not be NULL" or "the width must be positive".

If your function has any error conditions or does any error checking, you must describe how these errors are handled or indicated. For example, if your function calls malloc, describe how you handle a memory allocation failure. 

If your function has multiple arguments, it may be necessary to indicate the role of each argument, especially if the arguments are of the same type. Similarly, it may be necessary to describe what your function returns. A function returning an int may be returning the result of some computation, or true/false, or success/failure, or some other signal.

Within a function, self-documenting variable names (varaibles whose names describe their purpose) are preferable to brief names that get explained by a comment. Beware using the same or similar variable names (length, size..). A common exemption is loop variables, which are often named 'i'.

If a function can be logically broken into sections, add comments as section headers. If a function has many sections and subsections, it may indicate that the function should be broken into smaller pieces. 

Comments that repeat the plain meaning of code (e.g., "increase x by 1") are not helpful. If a line or section of code is particularly obscure or complex, a comment may be helpful to clarify, but rewriting the code to be simpler is better.
