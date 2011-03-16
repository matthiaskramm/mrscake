import sys
import re

VERBOSE = 0

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

# header detection

headers = None
NUMBER = re.compile("([0-9]+|[0-9]*[.][0-9]*(e[0-9]+)?)")
IDENTIFIER = re.compile("([a-zA-Z_][a-zA-Z0-9_]*)")
numbers_on_first_line = 0
numbers_on_second_line = 0
identifiers_on_first_line = 0
identifiers_on_second_line = 0
for item1,item2 in zip(data[0],data[1]):
    if NUMBER.match(item1):
        numbers_on_first_line += 1
    if IDENTIFIER.match(item1):
        identifiers_on_first_line += 1
    if NUMBER.match(item2):
        numbers_on_second_line += 1
    if IDENTIFIER.match(item2):
        identifiers_on_second_line += 1
if numbers_on_first_line < numbers_on_second_line and \
   identifiers_on_first_line > identifiers_on_second_line:
       print "has column headers"
       headers = data[0]
       data = data[1:]

# target column detection

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

if VERBOSE:
    if target_variable_is_last_column:
        print "The last column is the target variable"
    else:
        print "The first column is the target variable"

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

if headers:
    if target_variable_is_last_column:
        headers = headers[0:-1]
    else:
        headers = headers[1:]

import mrscake
dataset = mrscake.DataSet()
if headers:
    for inputs,output in examples:
        dataset.add(dict(zip(headers,inputs)), output=output)
else:
    for inputs,output in examples:
        dataset.add(inputs, output=output)

output_filename = "test.model"
model = dataset.get_model()
model.save(output_filename)

def show_model_performance(model, examples):
    correct = 0
    wrong = 0
    confusion = {}
    for inputs,output in examples:
        if output not in confusion:
            confusion[output]={}
        prediction = model.predict(inputs)
        if prediction not in confusion[output]:
            confusion[output][prediction] = 0
        confusion[output][prediction] += 1
        if prediction != output:
            wrong += 1
        else:
            correct += 1
    total = len(examples)
    print
    print "Confusion matrix:"
    rows = sorted(confusion.keys())
    columns = sorted(confusion.keys())
    print "p/r\t",
    for x,col_name in enumerate(columns):
        print "%s\t" % col_name,
    print
    for y,row_name in enumerate(rows):
        print "%s\t" % row_name,
        for x,col_name in enumerate(columns):
            print "%d\t" % (confusion.get(col_name,{}).get(row_name,0)),
        print
    print
    print "%2.2f%% accuracy (%2.2f%% error)" % ((correct*100.0 / (wrong + correct)), (wrong*100.0 / (wrong + correct)))
    print

show_model_performance(model, examples)

print "Model saved to",output_filename
