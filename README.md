# NilanModbus
Nilan ventilation cts 602 modbus communication

Using http for reading register group:

http://[device]/read/[group]

Showing groups

http://[device]/help 

Setting value

http://[device]/set/[group]/[address]/[value]


Sends group temp(also includes RH) to mqqt server every 3. minute.

Openhab items:

Switch nilanrun {mqtt=">[my:ventilation/RunSet:command:ON:1],>[my:ventilation/RunSet:command:OFF:0]"}<br/>
Number nilanrunstate {mqtt="<[my:ventilation/control/RunSet:state:REGEX(([0-9-]+))]"}<br/>
Number nilanmode      {mqtt=">[my:ventilation/ModeSet:state:*:default]"}<br/>
Number nilanmodestate {mqtt="<[my:ventilation/control/ModeSet:state:REGEX(([0-9-]+))]"}<br/>
Number nilanvent      {mqtt=">[my:ventilation/VentSet:state:*:default]"}<br/>
Number nilanventstate {mqtt="<[my:ventilation/control/VentSet:state:REGEX(([0-9-]+))]"}<br/>
Number nilantemp {mqtt=">[my:ventilation/TempSet:state:*:REGEX(([0-9-]+))]"}<br/>
Number nilantempstate {mqtt="<[my:ventilation/control/TempSet:state:REGEX(([0-9-]+))]"}<br/>
Number nilaninlet "Ventilation indgang temp [%s]" {mqtt="<[my:temp/T7_Inlet:state:REGEX(.*?([0-9.]+).*]"}<br/>
Number nilanoutside "Ventilation ude temp [%s]" {mqtt="<[my:temp/T8_Outdoor:state:REGEX(.*?([0-9.]+).*]"}<br/>


Openhab sitemap:

Frame label="Ventilation" {<br/>
                Switch item=nilanrun label="Tændt"<br/>
                Selection item=nilanmode label="Funktion" mappings=[0= "Off", "1"="Heat", "2" = "Cool", 3 = "Auto", 4 = "Service"]<br/>
                Selection item=nilantemp label="Temperatur" mappings=[1900="19", 2200="22", 2500="25"]<br/>
                Selection item=nilanvent label="Hastighed" mappings=[0="0",1="1",2="2",3="3",4="4"]<br/>
        }<br/>
