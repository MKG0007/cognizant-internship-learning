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
- it got its final update and no new feature will never be added.
- good for managing old lagecy code

.NET Core
- Cross-Platform(can be use in any operating system) 
- open source
- high performance and cloud optimized(removed ledacy baggage)
- moduler Design(use whatever you need in project)
- can be contained in the separate folder for each project
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



there are majoraly three types of datatypes in Csharp:


1)Value Type(value store in there own memory (in stack))

- int (use to store integer value)-32 bit
- long(use to store large numbers)-64 bit
- short(use to store short numbers)-16 bit
- float(use to store decimal numbers)-32 bit
- double(for large decimal numbers)-64 bit
- decimal(for very large numbers)-128-bit

- bool(use to store boolean value true/false)
- char(use to store the single character)
--------------------------------------------------------------------------
- structs(structures)
- it is the value type usually stored in stack.
- and stack is self managing when a variable goes out of scope, the memory is reclaimed instantly.
- use to reduce the overhead on the garbage for high-performance loops.
- use when object is small and immutable.
- example:
public struct Point
{
    public int X;
    public int Y;

    // Constructor
    public Point(int x, int y)
    {
        X = x;
        Y = y;
    }
    public void Display() => Console.WriteLine($"X: {X}, Y: {Y}");
}
- to access
Point stj = new Point(3 , 5);
stj.x
stj.display();

- access without "new"
Point stj;
stj.x
stj.display()
- this thing only works when each and every variable inside struct is intiallized.

- Using Object Initializer Syntax
short way to assign the values to the variables in one go without using constructor
var p = new Point { X = 100, Y = 200 }

--------------------------------------------------------------
- enums(enumerations)
- set of name constants(aliases for integers)
- starting from 0
- example:
public enum OrderStatus
{
    Pending,    // 0
    Processing, // 1
    Shipped,    // 2
    Delivered,  // 3
    Cancelled   // 4
}
- to access
OrderStatus.shipped


2)Reference types(they store a reference to the memory address where the data shored(in heap memory))

- string(use to store text)
it is immutable in C#

- object(ultimate base class)

- arrays(use to store collection of variables)
- classes(User-defined blueprints for objects)
- interfaces(it only provides the structure to the class)




conditional statements
- if , else if , else
- switch
- condition ? value_if_true : value_if_false;(Ternary Operator)

loops in C#
- for loops(to take full control over iterator)
- foreach loop(use to iterate the collection without using counter)
- while loop(checks the condition before running code)
- do while loop



Array in C#
- it is a collection of same datatypes in one variable
- fixed size and index start from zero
syntax:
int[] num = new int[size];
string[] arr = {element}

for 2d array
int[,] matrix = new int[row , col]

array methods:
- Length(return length of the array)
- Rank(return the total dimension of the array)




String in C#
- it is the collection of characters.
- it is the reference data type
- it is immutable
- we can not make update/changes int the value of the string but we can re-assigned that value
- Length(to get the length)


methods of string:
- Contains(value) return true if the value is present
- IndexOf(value) return the first occurance position
- ToUpper() | ToLower() convert the entire string to uppercase and lowercase
- Trim()/TrimStart()/TrimEnd() remove end and start whitespace
- replace(old , new) replaces all occurances of a string with another
- Substring(startIndex , length) return the part of the string
- split(string , "word") return array that contain splited parts according to the "word"
- String.Join("word" , arr) return the string created by adding array element with "word"

- string.Format("here we can write sentences with variable")




object oriented programming in C#

class(user defined blueprint of object)
object(instance of class that represent real world entities)

basic syntax:

// The Blueprint
public class Dog 
{
    // data members and data funtions(methods)
    // function inside the class called methods
    // variables represent the state of the object
    public string Breed;
    public int Age;

    public void Bark() 
    {
        Console.WriteLine("Woof! Woof!");
    }
}

object creation(ClassName obj = new ClassName();)



example:

using System;
public class Smartphone{
    
    public string Brand;
    public string Model;
    public void MakeCall(){
        Console.WriteLine($"Call is done by {Brand} for {Model}.");
    }
}

public class HelloWorld
{
    public static void Main(string[] args)
    {
        Smartphone obj1 = new Smartphone{Brand = "zolo" , Model = "mayfield"};
        Smartphone obj2 = new Smartphone{Brand = "holo" , Model = "darkfield"};
        
        obj1.MakeCall();
        obj2.MakeCall(
            );
    }
}


constructors:
- it is use to intialized the object
- it called as the time we use "new" keyword
- It has the exact same name as the class.
- It has no return type (not even void).

example:
public class BankAccount 
{
    public string Owner;
    public decimal Balance;

    // This is the Constructor
    public BankAccount(string name, decimal initialDeposit) 
    {
        Owner = name;
        Balance = initialDeposit;
    }
}


Access modifiers:
- public(access from any where) 
- private(access within the class only)


Properties(the smart gate):
- permisson declaration
- get(what and how should we return the requested data)
- set(which data is allowed for the change)
- for each variabe we have to define separate "get , set" block.
- even we are using "get set" we can also use constructor too.

example:
short way: public string Username{get; set;}

private int _age; // The actual data (hidden in the vault)
public int Age 
{
    get 
    { 
        // Logic: What happens when someone types: 				Console.WriteLine(myObj.Age);
        return _age; 
    }
    set 
    { 
        // Logic: What happens when someone types: myObj.Age = 25;
        // 'value' is a special keyword that holds the incoming number (25)
        if (value > 0) 
        {
            _age = value; 
        }
    }
}

- if we only mention single one from (get , set) then is work as permission.

naming convention:
private field(use camelCase)
public property(use PascalCase)



inheritance in C#
- it allows one class inherit the properties another class.
base class(parent)
derived class(child)


public class animal{}//parent 
public class lion:animal{}// child

note:
- base(arg) //it is use to pass the parent argument from the child class constructor.



polymorphism in c#
- means "many shapes"

1) overriding(overriding the parent function by the child)

- override means overriding the virtual function in the child class
- virtual means add there which will going to change but the child class

example:
public class Animal
{
    public virtual void MakeSound()
    {
        Console.WriteLine("The animal makes a generic sound.");
    }
}

public class Dog : Animal
{
    public override void MakeSound()
    {
        Console.WriteLine("The dog barks: Woof! Woof!");
    }
}

public class Cat : Animal
{
    public override void MakeSound()
    {
        Console.WriteLine("The cat meows: Meow!");
    }
}

explained use:
List<Animal> zoo = new List<Animal>();
zoo.Add(new Dog());
zoo.Add(new Cat());

foreach (Animal a in zoo)
{
    a.MakeSound(); 
    // Output:
    // "The dog barks: Woof! Woof!"
    // "The cat meows: Meow!"
}


2)method overloading in C#
- when the there is two methods with the same name but different bihavour
- there is must be some difference in parameter(like number or type)

example:
public class Calculator
{
    // Version 1: Adds two numbers
    public int Add(int a, int b) { return a + b; }

    // Version 2: Adds three numbers
    public int Add(int a, int b, int c) { return a + b + c; }
}




abstract classes:
- blue print of class
- we can not create objects of abstract class
- we can have abstract methods with no code body
- it can have variables too
- can have constructor
- access modifiers is there 
example:
public abstract class Appliance
{
    // Every appliance must TurnOn, but they all do it differently
    public abstract void TurnOn(); 

    // Abstract classes can still have regular methods too!
    public void PlugIn() 
    {
        Console.WriteLine("Appliance is plugged in.");
    }
}

public class Toaster : Appliance
{
    public override void TurnOn() 
    {
        Console.WriteLine("Heating up coils...");
    }
}



interfaces in C#
- it is use for multi-inheritence because normal can only inherit one parent class only.
- naming of interface starts with "I"
- An Interface is a completely "abstract type" that defines a set of related functionalities that a class (or a struct) must implement.
- it can only have method without body and can not have variables
- no constructor too
- it is generally public by default

example:
public interface IRecoverable
{
    void Save(); // No code here, just a "must-have"
}

public interface IPrintable
{
    void Print();
}

// This class follows TWO contracts
public class Document : IRecoverable, IPrintable
{
    public void Save() { Console.WriteLine("Saving to Disk..."); }
    public void Print() { Console.WriteLine("Sending to Printer..."); }
}



static and constants in C# classes:

- when we use "new" that create separate unique copy of data
but if there is any need to have something that is shared by everyone? That is where static comes in.

static keyword
- static members belongs to the class itself , not to any class object
- there is no need to use "new" to access it

public class Player
{
    public string Name; // Unique to every player
    public static int PlayerCount; // Shared by ALL players

    public Player(string name)
    {
        Name = name;
        PlayerCount++; // Every time a new player is born, the shared counter goes up
    }
}

public class hello{
	public static void main(){
		Player p1 = new Player("Alice");
		Player p2 = new Player("Bob");

		Console.WriteLine(Player.PlayerCount); // Output: 2
		// Notice we used "Player.PlayerCount" NOT "p1.PlayerCount"
	}

}

constant keyword
- it is on the value that not going change.

example:
public class Physics
{
    public const double Gravity = 9.8;
}




Generics in C#(the universal type template)
- use <T> instead of writting the datatype

Generic classes

example:
// The Generic Blueprint
public class Box<T> 
{
    private T _content;

    public void Add(T item) 
    {
        _content = item;
        Console.WriteLine("Item added to the box.");
    }

    public T GetItem() 
    {
        return _content;
    }
}

// Using the Generic Class
Box<int> numberBox = new Box<int>();
numberBox.Add(123);

Box<string> nameBox = new Box<string>();
nameBox.Add("Gemini");

- code become reusable 




