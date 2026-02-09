C# Programming language:
- journey of proprietary systems to open Source
-C# is a modern, object-oriented, and type-safe programming language developed by Microsoft.
- It runs on the .NET platform and is one of the most versatile languages in existence today.
- type-safe(strict rules even the tiny mistake gives error)
- memory management(have the Garbage Collector for auto clean up)




compilation process(two step compilation)

phase-1 compile Time
- where, Source Code convert into MSIL(Microsoft Intermediate Language) by Roslyn Compiler


Phase-2 run time
-  goes to Common Language Runtime(CLR)

inside the CLR
step-1- code goes to JIT(just in time) compiler that convert MSIL code into Machine code

step-2- native code stored in the memory and it will run when you call it again wihout any more process.

within this whole process
- CLR also creates the Managed Evironment

that manages:
1)Garbage Collection
2)Type Saftey(prevent unauthorized access)
3)Exception Handling



.NET Framework 
- proprietary system(close scope)
- Global installation(all app shares the common installation)
- Tightly Coupled to Windows(it depend on many windows-specific components)
- Monolith in nature(even if you only needs the tiny bit code entire framework must be present)
- it got its final update and no new feature will ever be added.
- good for managing old lagecy code

.NET Core
- Cross-Platform(can be use in any operating system) 
- open source
- high performance and cloud optimized(removed ledacy baggage)
- moduler Design(use whatever you need in project)
- can be contained in the separate folder for each 
- actively developing by the developers


comments in C#
- // comments(for single line)
- /* */ (for multi line)


first code:

using System; //include the libirary named System

public class HelloWorld //C# code must written inside the class

{
    public static void Main(string[] args) // main function from where the code starts executing

    {
        Console.WriteLine ("Try programiz.pro"); //to print line into the terminal
    }

}




C# input and output

Console.Write(string) // print the text in the same line
Console.WriteLine(string) // print the text in the new Line

Console.ReadLine() //to take input from the user
//it always return input in string
// so we have to convert the value into needed data type:

int.Parse(Console.WriteLine());


note:
string interpolation is possible in C#
- Console.WriteLine($"hello {v1} yoho {v2}");

- Console.ReadKey()// press any to exit
- Console.ReadKey(true) // this way the key you press will not shown into the screen
- this thing programmer use to see the result by adding this as a breaker












