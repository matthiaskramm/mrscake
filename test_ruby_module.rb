require 'prediction'

data = Prediction::DataSet.new

data.add([0.3,1.0,:bla], :yes)
data.add([0.4,1.0,:bla], :yes)
data.add([0.5,0.0,:bli], :no)
data.add([0.3,0.5,:bla], :no)
data.add([0.4,1.0,:bli], :yes)
data.add([0.6,0.0,:bla], :no)

data.print

model = data.get_model()
model.print

model.save("/tmp/model.dat")
model = Prediction::load_model("/tmp/model.dat")

model.print

p model.predict([0.3,1.0,:bla])
p model.predict([0.1,0.0,:bli])
p model.predict([0.2,0.5,:bla])
p model.predict([0.1,1.0,:bli])
p model.predict([0.1,1.0,:blo])

puts model.generate_code("python")

data = Prediction::DataSet.new

data.add({:bananas=>0.3,:oranges=>1.0,:name=>:bla}, :yes)
data.add({:bananas=>0.4,:oranges=>1.0,:name=>:bla}, :yes)
data.add({:bananas=>0.5,:oranges=>-1.0,:name=>:bli}, :no)
data.add({:bananas=>0.3,:oranges=>-1.0,:name=>:bla}, :no)
data.add({:bananas=>0.4,:oranges=>1.0,:name=>:bli}, :yes)
data.add({:bananas=>0.6,:oranges=>-1.0,:name=>:bla}, :no)

data.print

model = data.get_model()
model.print

model.save("/tmp/model.dat")
model = Prediction::load_model("/tmp/model.dat")

model.print

p model.predict({:bananas=>0.3,:oranges=>1.0,:name=>:bla})
p model.predict({:bananas=>0.4,:oranges=>-1.0,:name=>:bli})
p model.predict({:bananas=>0.5,:oranges=>1.0,:name=>:bli})
p model.predict({:bananas=>0.8,:oranges=>-1.0,:name=>:bla})
p model.predict({:bananas=>0.9,:oranges=>1.0,:name=>:bla})

puts model.generate_code("python")

