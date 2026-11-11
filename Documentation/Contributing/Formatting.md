# Code formatting

## Naming

* Functions & Classes in PascalCase (like `MyFunction` or `MyClass`)
* Put `m_` before member variables + PascalCase (like `m_EngineData`) but not functions
* Put `p_` before parameters + camelCase (like `p_outputData`) but not in front of very short names
* Local variables in camelCase

## Things to remember

* Make sure to add `const` to member functions where applicable
* Add `[[nodiscard]]` to functions with return types that will be wasted if their return value is not used
* Add `inline` to small helper functions
* If using both `[[nodiscard]]` and `inline`, put both on the line above the function. For example:
```cpp
[[nodiscard]] inline
int Add(int a, int b) {
	return a + b;
}
```

Make sure to:
* Remove duplicated functions, variables, includes or more from your code before publishing
* The code aligns to the formatting rules
* Add comments (below)

### Comments

Make sure to:
* Add in your code
* State clearly as what the code does if it's unclear at a first glance
* Try to keep them as short as possible without making them unclear
* Use only single-line comments, like:
```cpp
// Hey! this is my comment
// New line because it's too big!
```
> Make them multiple lines if they are too big

## Indenting & Newlines

* Make sure to add newlines at the end of files to avoid warnings or bugs

### Braces

* Last brace needs to be on the next line of the end of the code
* Only make the opening brace on the next line if the body (or code inside) is more than one line
For example:
```cpp
void Add(int a, int b) { // <- Same line
  return p_A + p_B;
}

int main()
{ // <- New line
  // Many lines of code

  for(uint32_t i = 0; i < 32; i++) { // <- Same line, only one line of code inside
    // Calculations
  }
}
```
> I did this because I felt like it took up too much space with newlines for many one-line functions, and they are eitherways readable 

## A complete example

```cpp
class MathUtility {
private:
    int m_LastResult;

public:
    // Get the sum of 2 numbers
    [[nodiscard]] inline 
    int ComputeSum(int p_FirstNumber, int p_SecondNumber)
    {
        int sumResult = p_FirstNumber + p_SecondNumber;
        m_LastResult = sumResult;
        return sumResult;
    }

    // Gets the last result the user calculated
    [[nodiscard]] inline 
    int GetLastResult() { 
        return m_LastResult; 
    }

    // Processes and returns using an internal algorithm
    [[nodiscard]] int ProcessComplexCalculation(int p_InputValue)
    {
        int runningTotal = p_InputValue * 2;
        runningTotal += 10;
        m_LastResult = runningTotal;
        return runningTotal;
    }
};
```
