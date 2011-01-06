import predict

data = predict.DataSet()
data.train([1,2.0,5,1.0,"BLA",7], output=1)
data.train([1,2.0,5,0.0,"BLA",7], output=1)
data.train([1,2.0,5,0.0,"BLB",7], output=2)
data.train([1,2.0,5,0.0,"BLB",7], output=2)
data.train([1,2.0,5,0.0,"BLC",7], output=3)
data.train([1,2.0,5,0.0,"BLC",7], output=3)
data.train([1,2.0,5,0.0,"BLD",7], output=4)
data.train([1,2.0,5,0.0,"BLD",7], output=4)
print data
model = data.get_model()
print model
#model.save("/tmp/model.dat")
#model = predict.load_model("/tmp/model.dat")
print model
print model.predict([3,2.0,7,1.0,"BLA",1])
print model.predict([3,2.0,7,1.0,"BLB",1])
print model.predict([3,2.0,7,0.0,"BLC",1])
print model.predict([3,2.0,7,0.0,"BLD",1])
