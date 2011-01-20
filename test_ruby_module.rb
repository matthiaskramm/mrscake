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

