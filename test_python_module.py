import predict

data = predict.DataSet()
data.train([1,2.0,5,3.0,7], output=1)
data.train([1,2.0,5,4.0,7], output=2)
data.train([1,2.0,5,5.0,7], output=3)
data.train([1,2.0,5,6.0,7], output=4)
print data
model = data.get_model()
print model
model.save("/tmp/model.dat")
model = predict.load_model("/tmp/model.dat")
print model
print model.predict([3,2.0,7,3.0,1])
print model.predict([3,2.0,7,4.0,1])
print model.predict([3,2.0,7,5.0,1])
print model.predict([3,2.0,7,6.0,1])
