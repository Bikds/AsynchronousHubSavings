# AsynchronousHubSavings

Description of repository:

hub_config.c - This is the C file which does the simulation. It takes as input hub_config.in and uses constants defined in hub_config.h. The code contains our proposed algorithm and outputs the savings that our algorithm would achieve.

hub_config.in - The specification of the USB Hub setup. First line specifies the number of lanes in, the number of lanes out, and the speed of the link. The second line indicates how many DFPs there are. The lines beyond the second line contain the specification of the DFP, consisting of USB version, number of lanes in, number of lanes out, speed of the link, upstream bandwidth demand, and downstream bandwidth demand.

hub_config.h - Definition of the constants/values that are used in hub_config.c.

Instructions to run the code.

1. Compile the C code (gcc -o \<executable name\> hub_config.c)
2. Run the executable (./\<executable name\>)
