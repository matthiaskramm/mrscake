import predict

data = predict.DataSet()
data.train([1,2.0,5,1.0,"A",7], output="1")
data.train([1,2.0,5,1.0,"B",7], output="2")
data.train([1,2.0,5,0.0,"C",7], output="3")
data.train([1,2.0,5,0.0,"D",7], output="4")
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
