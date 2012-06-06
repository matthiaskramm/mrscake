import mrscake

data = mrscake.DataSet()
data.add([2,1.0,5,1.0,"A",7], output="yes")
data.add([1,1.3,3,1.0,"B",7], output="yes")
data.add([2,0.3,3,0.0,"A",3], output="no")
data.add([1,1.1,3,0.5,"C",7], output="no")
data.add([2,0.7,3,1.0,"A",4], output="yes")
data.add([2,0.7,3,1.0,"D",7], output="bla")
data.add([1,0.6,3,1.0,"A",7], output="bla")
data.add([1,0.6,3,2.0,"B",8], output="bla")
data.add([1,0.6,3,1.0,"A",9], output="bla")
data.add([2,0.7,3,1.0,"A",7], output="yes")
data.add([1,1.1,5,0.0,"B",10], output="no")

data.save("/tmp/data.txt")
data = mrscake.load_data("/tmp/data.txt")

print data
model = data.train()

print model

model.save("/tmp/model.dat")
model = mrscake.load_model("/tmp/model.dat")
print model
print model.generate_code("python")
print model.predict([3,2.0,7,1.0,"A",1])
print model.predict([3,2.0,7,1.0,"B",1])
print model.predict([3,2.0,7,0.0,"C",1])
print model.predict([3,2.0,7,0.0,"D",1])

