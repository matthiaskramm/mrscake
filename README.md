About:
------

mrscake (read: "Mrs. Cake") is a machine learning library that automatically picks the best[1] model for you.
It can also generate code. 
Generated code might look like this:

```python
#### GENERATED CODE ####
def predict(aquatic, domestic, eggs, backbone, feathers, 
            predator, airborne, hair, toothed, tail, 
            breathes, catsize, venomous, legs, fins, milk):
    if eggs == "no":
        return "mammal"
    else:
        if backbone == "yes":
            return "bird"
        else:
            if aquatic == "no":
                return "insect"
            else:
                return "crustacean"
```


Compiling:
----------

Compile it using

```bash
    ./configure
    make
    make install
```
.

It has a Ruby and a Python interface.

Usage:
------

### Python

```python
import mrscake
data = mrscake.DataSet()
data.add(["a", 1.0, "blue"], output="yes")
data.add(["a", 3.0, "red"], output="yes")
data.add(["b", 2.0, "red"], output="no")
data.add(["b", 3.0, "blue"], output="no")
data.add(["a", 5.0, "blue"], output="yes")
data.add(["b", 4.0, "blue"], output="yes")
model = data.train()

result = model.predict(["a", 2.0, "red"])

code = model.generate_code("python") # or: ruby, javascript, c

print code
```

### Ruby

```ruby
require 'mrscake'
data = MrsCake::DataSet.new
data.add([:a, 1.0, :blue], :yes)
data.add([:a, 3.0, :red], :yes)
data.add([:b, 2.0, :red], :no)
data.add([:b, 3.0, :blue], :no)
data.add([:a, 5.0, :blue], :yes)
data.add([:b, 4.0, :blue], :yes)
model = data.train()
model.print

result = model.predict([:a, 2.0, :red])

code = model.generate_code("ruby") # or: ruby, javascript, c

puts code
```

<hr>

[1] Mrscake picks a model by an information-theoretic approach called MDL: It picks the model with the shortest description length. I.e., from a code generation standpoint, it gives you the shortest piece of code that would recognize all[2] the examples in your training set. (Also known of the Kolmogorov complexity of the labels, given the feature data)
[2] Yes, all. It internally builds a program consisting out of the classifier it returns as well as special cases for all the incorrectly labeled elements in the training set.

