

function testGarbageCollector(time)
{
	if(typeof time != "number")
		return false

	for(var startTick = GetTickCount(); GetTickCount() < startTick + time;) {
		var obj = new CreateTestObject(5, 5)

		obj.x = 12
		var sum = obj.sum()

		DeleteTestObject(obj)
		delete obj
	}

	return true
}

function testCallNTimes(times)
{
	var i=0

	callNTimes(times, function() { 
		i++
	})

	return times == i
}

function loop()
{
	gc();
	
	print("V8-Tests will be started...\n")

	print("The garbage collector will be tested...")
	testGarbageCollector(50)
	print("The garbage collector has been successfully tested.")

	print("\n")

	print("CallNTimes will be tested...")
	var res = testCallNTimes(100)
	if(res)
		print("CallNTimes has been successfully tested")
	else
		print("CallNTimes failed!")

	print("\n")

	print("Test sleep of one second (TickCount : " + GetTickCount() + ")")
	sleep(50)
	print("Test sleep finished (TickCount: " + GetTickCount() + ")")

	print("\n")

	print("Testing threads...")

	
}

function main()
{
	TimedThread(1000, function(){
		print("thread")
	})
}

main();
for(;;/*loop()*/);