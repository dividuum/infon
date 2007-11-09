return {[[
How can I detect a map change?

Define the global function onGameStart(). 
It will be called once a new map started.
]]
,---------------------------------------
[[
Did you know you can extend the available 
commands?

You can redefine the global function 
onCommand(line). It will be called with 
any input typed in the client unknown to 
infon.
]]
,---------------------------------------
[[
Did you know you can load additional code
into your vm?

You can use dofile(source) to load files
into you vm. To see a list of available
files, call dofile_list() within your vm.
]]
,---------------------------------------
[[
By entering the values 0 upto 9 in the
commandline, you call the functions
onInput0 upto onInput9. You might use
them to change the strategy your bot 
uses without much typing.
]]}

