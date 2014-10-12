print("asdf")

print(JSON.stringify({id: 0}));

callNTimes(5, function(){
	print("Callback")
})

try {
	callNTimes()
}
catch(error) {
	print(error)
}

try {
	callNTimes(5, 6)
}
catch(error) {
	print(error)
}