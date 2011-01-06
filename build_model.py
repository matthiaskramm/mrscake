import sys
import re

def is_float(s):
    try:
        int(s)
        return False
    except:
        pass
    try:
        float(s)
        return True
    except:
        return False

def convert_value(s):
    try:
        return int(s)
    except:
        try:
            return float(s)
        except:
            return s

CSV_FILE = re.compile(r"^[^,]+,[^,]+.*")
TSV_FILE = re.compile(r"^[^\t]+\t[^\t]+.*")

filename = "data/segment.dat"

if len(sys.argv) > 1:
    filename = sys.argv[1]

csv_score = 0
tsv_score = 0
ssv_score = 0
with open(filename, "rb") as fi:
    for line in fi.readlines():
        if CSV_FILE.match(line):
            csv_score += 1
        elif TSV_FILE.match(line):
            tsv_score += 1
        else:
            ssv_score += 1

file_type = max([(csv_score, "csv"), (tsv_score, "tsv"), (ssv_score, "ssv")])[1]

seperator_char = ","
if file_type == "csv":
    seperator_char = ","
elif file_type == "ssv":
    seperator_char = " "
elif file_type == "tsv":
    seperator_char = "\t"

data = []
with open(filename, "rb") as fi:
    for line in fi.readlines():
        line = line.strip()
        items = []
        inside_quotes = 0
        for item in line.split(seperator_char):
            if not inside_quotes:
                if item.startswith('"'):
                    item = item[1:]
                    inside_quotes = 1
                items.append(item)
            else:
                if item.endwith('"'):
                    item = item[:-1]
                    inside_quotes = 0
                items[-1] += seperator_char + item
        data.append(items)

first_column_values = {}
last_column_values = {}
first_column_floats = 0
last_column_floats = 0

number_of_items = {}
for row in data:
    first_column_values[row[0]] = None
    last_column_values[row[-1]] = None
    if is_float(row[0]):
        first_column_floats += 1
    if is_float(row[-1]):
        last_column_floats += 1
    number_of_items[len(row)] = number_of_items.get(len(row), 0) + 1

number_of_columns = sorted(number_of_items.items(),lambda a,b: cmp(a[1],b[1]))[-1][0]

target_variable_is_last_column = \
    last_column_floats < first_column_floats or \
   (last_column_floats == first_column_floats and \
    len(first_column_values) < len(last_column_values))

examples = []
for i in range(len(data)):
    row = data[i]
    if len(row) != number_of_columns:
        print "Ignoring row %d (too few entries, %d/%d)" % (i, len(row), number_of_columns)
    if target_variable_is_last_column:
        output = row[-1]
        inputs = row[0:-1]
    else:
        output = row[0]
        inputs = row[1:]
    inputs = map(convert_value, inputs)
    output = convert_value(output)
    examples += [[inputs,output]]

import predict
dataset = predict.DataSet()
for inputs,output in examples:
    dataset.train(inputs, output=output)

model = dataset.get_model()
print model

correct = 0
wrong = 0
for inputs,output in examples:
    prediction = model.predict(inputs)
    if prediction != output:
        wrong += 1
    else:
        correct += 1

print "%2.2f%% accuracy (%2.2f%% error)" % ((correct*100.0 / (wrong + correct)), (wrong*100.0 / (wrong + correct)))

