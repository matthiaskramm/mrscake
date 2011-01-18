import predict

data = predict.DataSet()
#data.train([2,1.0,5,1.0,"A",7], output="yes")
#data.train([1,1.3,3,1.0,"B",7], output="yes")
#data.train([2,0.3,3,0.0,"A",7], output="no")
#data.train([1,1.1,3,0.5,"C",7], output="no")
#data.train([2,0.7,3,1.0,"A",7], output="yes")
#data.train([1,1.1,5,0.0,"B",7], output="no")

data.train([1.0], output="yes")
data.train([1.0], output="yes")
data.train([0.0], output="no")
data.train([0.5], output="no")
data.train([1.0], output="yes")
data.train([0.0], output="no")
print data
model = data.get_model()
print model
model.save("/tmp/model.dat")
model = predict.load_model("/tmp/model.dat")
print model
print model.predict([3,2.0,7,1.0,"A",1])
print model.predict([3,2.0,7,1.0,"B",1])
print model.predict([3,2.0,7,0.0,"C",1])
print model.predict([3,2.0,7,0.0,"D",1])

