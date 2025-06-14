# ME507 MECHA07 TERM PROJECT # {#mainpage}

### Hello! Welcome to our Mechatronics Term Project.  Created by Evan Long and Justin Lesko.

The following content documents our completion of Cal Poly's ME507 Mechatronics Term Project.  The overall goal of the
 project was to select a personal project that requires automation and design a custom PCB to accomplish the task.
 Our project is an automatic cat feeder.  The user will place a large quantity of dry cat food in the storage container
 of the feeder, and the feeder will automatically dispense the food at the appropriate time for the cat.  Over the course
 of the spring 2025 quarter, we designed the cat feeder and custom PCB, ordered the parts, wired everything up, and 
 programmed the feeder to create a functional final product.

The cat feeder dispenses food with a large lead screw, and has a pair of load cells to measure the weight of food in the
 bowl.  The feeder is equiped with a wifi module and is programmed to act as an http server.  With port forwarding set up
 on a local network, the user can use any browser to access the feeder controls from anywhere with internet access.
 As a result, the user can be out of town and still log in to feed the cat.

The goal of this website is to document our efforts complete the project.  On this page, we include an overview of our 
 mechanical, electrical, and software designs.  Also included on this page is navigation to more in depth documentation
 of the rest of our software.

 A link to our project github can be found here: [GITHUB](https://github.com/Cal-Poly-ME507-MECHA07/Cal-Poly-ME507-MECHA07.github.io)

 <pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/Feeder%20Picture.jpg" style="width: 50%; height: 50%">

Figure 1.  Image of the cat feeder.

<pre>

</pre>

\htmlonly
<iframe width="560" height="315" src="https://www.youtube.com/embed/INiQte2DkHs?si=SLNbhLrZdzP5DMYY" title="YouTube video player" allowfullscreen></iframe>
\endhtmlonly

Figure 2.  Video of the cat feeder dispensing food

<pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/Website%20Screenshot.png" style="width: 50%; height: 50%">

Figure 3.  Screenshot of the cat feeder website.

<pre>



</pre>

### Mechanical Design

The mechanical structure was constructed primarily with aluminum and acryllic sheets that are tabbed and slotted together.
 The primary dispensing action is accomplished with a 3D printed lead screw powered by a geared stepper motor.  To assist
 the filling, there is a second stepper motor with a brush flap to push the cat food all the way into the bowl.

The screw motor is a NEMA23 stepper motor with 1.2 N-m of torque connected to a 20:1 planetary gearbox.  The flap motor is
 a NEMA17 stepper motor with 0.52 N-m of torque and an integrated 1000PPR encoder.  Both motors are set with a current limit
 just under 2A, although we suspect that due to the resistance of traces on our board used for current limit feedback control
 we may not be getting the full torque.  The encoders are optical encoders with a quadrature output.

The load cells are cheap 5kg parallel beam load cells for hobby applications.  We found them to reliably measure fast changes
 in weight, but the cells tend to drift over time, making it impossible to track how much food is actually remaining in the bowl.
 To compensate, we reset the calibration offset before every dispense.  As a result, the system is open loop in the sense that
 it will dispense more food even if there is currently food in the bowl.  To overcome this, we would either need higher quality
 load cells or some other sensor to detect the fill level.

 <pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/CAD%20Screenshot.png" style="width: 50%; height: 50%">

Figure 4.  Isometric view of the cat feeder 3D model.

<pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/Lead%20Screw.png" style="width: 50%; height: 50%">

Figure 5.  Isometric view of the cat feeder lead screw.  The lead screw was printed in three pieces and assembled together.

<pre>



</pre>

### Electrical Design

All the electronic circuitry is designed into the custom PCB.  The PCB accepts 12V power from an external power supply,
 and provides terminal block interfaces to connect 2 stepper motors, 2 encoders, and 2 load cells.  The wifi module is
 connected directly to the PCB via female headers.  The PCB contains a power regulation circuit, a STM32F411 
 microcontroller (MCU), stepper motor drivers, an ADC IC, and a calendar IC.  

The PCB was fabricated and assembled by an online PCB manufacturing service.  The board and power supply are installed 
 underneath the feeder, and the motors and load cells are wired directly the board with the requirement that no wires can be 
 exposed on the outside of the feeder, to prevent the cat from chewing the cables.  The board was programmed with an ST-Link
 module connected via pin headers.

The power regulation circuit accepts 12V from the external power supply.  The first switching regulator provides a noisy
 5.5V output, which is fed into two LDO regulators which provide clean 5V and 3.3V, respectively.

The microcontroller has a high speed ceramic crystal, a reset button, and all the required decoupling capacitors.  We also
had some extra space on the board to break out some extra pins into pin headers, but luckily we never ended up needing them.

The stepper motor drivers accept step and direction input from the MCU.  We wired the drivers such that the current sense
 feedback resistors are externally inserted into female headers, so we can adjust the current limits without resoldering.
 The drivers are set to decay the current with synchronous rectification to minimize noise.

The ADC IC communicates with the MCU via SPI.  Both load cells are wired to the one ADC, so the MCU commands the ADC to switch
 the inputs for every reading so both load cells can be polled regularly.  The load cells are wired with filtering circuitry
 included on the board, and the ADC provides a filter function as well, but we still found occasional noise in the outputs.

The calendar IC is used to keep track of long periods of time when relying on the high speed crystal would likely cause error.
 The IC has an external 32.768kHz crystal to regulate time.  It communicates with the MCU via I2C.

The wifi module is a complete breakout board with an integrated MCU to handle wifi connections designed for hobby applications.
 The wifi module is installed on the board via female headers.  It communicates with the MCU via UART.  To interact with 
 the module, the MCU sends AT commands over UART.

 <pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/PCB%20Assembled.jpg" style="width: 50%; height: 50%">

Figure 6.  Image of the assembled PCB.

<pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/PCB%20Installed.jpg" style="width: 50%; height: 50%">

Figure 7.  Image of the installed PCB and power supply.

<pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/MCU%20Schematic.jpg" style="width: 50%; height: 50%">

Figure 8.  Schematic of the microcontroller circuitry.

<pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/Power%20Schematic.jpg" style="width: 50%; height: 50%">

Figure 9.  Schematic of the power supply circuitry.

<pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/ADC%20Schematic.jpg" style="width: 50%; height: 50%">

Figure 10.  Schematic of the ADC circuitry.

<pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/PCB%20Screenshot.png" style="width: 50%; height: 50%">

Figure 11.  Screenshot of the PCB design.

<pre>



</pre>

### Software Design

The software installed on the MCU has two goals.  The first goal is dispensing the correct amount of food into
 the bowl when required.  The second goal is hosting an http server to accept user inputs.  The software is written in c and
 is divided into two classes: Filler and Interface, which handle the filling and the server, respectively.

A link to our project github can be found here: [GITHUB](https://github.com/Cal-Poly-ME507-MECHA07/Cal-Poly-ME507-MECHA07.github.io)

The software provides a user interface over the hosted http website. The website displays the current enabled status 
 of the device and allows the user to toggle the state.  It also displays the current and target weights.  There are three 
 main functional modes: feed once, maintain weight, and timed feeding. For timed feeding, the website allows the user to 
 enter a desired amount of hours and minutes from the current time that they wish for the cat feeder to run. Feed once will 
 dispense the right amount of food, then disable the device.  Maintaining the weight will attempt to keep the level of the 
 food bowl constant, but we have found that due to the slow drift of our load cells this function is only reliable for short 
 periods of time.  Also on the website are buttons to manually advance the screw or  cycle the flap, mostly for testing.

Naturally, the 96MHz MCU speed is overkill for the speed that the cat feeder needs to update its outputs.  As
 a result, we have the freedom to complete the control logic with simple Finite State Machines (FSM).  The filling action
 is accompished with two FSMs.  The first FSM is responsible for reading the load cell values from the ADC.  The second 
 FSM is responsible for updating the stepper motor speeds at the appropriate time.

The largest challenge of the software was getting the http server working.  The server is entirely embedded within the
 MCU and requires no external computing.  When a browser attempts to connect to the IP address of the wifi module, the wifi 
 module forwards the request over UART to the MCU.  The MCU stores the request in a buffer, then searches the buffer for
 keywords.  If it recognizes the request, it sends a simple http website back to the wifi module which in turn gets forwarded
 to the browser.

The main function first initializes the interface, then the ADC, and finally starts the main loop.  The main loop then calls
 the run_interface function, which checks for UART commands from the ST-Link and connection requests from the wifi module,
 responds with the appropriate data, and handles alarms set by the user.  The run_interface function is non-blocking.  
 Then the main loop calls the run_filler function, which runs the ADC FSM and the filling FSM.  The main loop is slowed to a 
 speed of 100Hz to allow time for the wifi module uart connection to fill in the buffer.

A top priority of this project is maximizing reliability.  If the user is out of town, the device needs to remain functional
 over long periods of time without any errors, to ensure the cat remains fed.  To clarify, the device doesn't need to be functional 
 100% of the time, but it does need to recover after every error.  As a result, we have implemented code to automatically reset the
 device every 5 minutes after saving all key values in flash memory.  As a result, in the event of a software error or glitch in one
 of the peripherals, the user only needs to wait 5 minutes to reestablish functionality.

To maximize compatibility with the STM32CubeIDE, the software was written in c.  The software follows a pseudo object oriented
 style, with each file having 1-2 structs and a series of local functions for interacting with the structs.

 <pre>

</pre>

 <img src="https://cal-poly-me507-mecha07.github.io/Main%20FSM.jpg" style="width: 50%; height: 50%">

Figure 12.  State space diagram of the main FSM.

<pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/Wifi%20FSM.jpg" style="width: 50%; height: 50%">

Figure 13.  State space diagram of the wifi initialization FSM

<pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/ADC%20FSM.jpg" style="width: 50%; height: 50%">

Figure 14.  State space diagram of the ADC reading FSM

<pre>

</pre>

<img src="https://cal-poly-me507-mecha07.github.io/Filling%20FSM.jpg" style="width: 50%; height: 50%">

Figure 15.  State space diagram of the filling FSM

<pre>



</pre>

### Challenges

Due to the scope of the project and the timeline we had it was inevitable that we would run into challenges along the way.
 Here, we document some of the most important challenges to complete the project documentation and demonstrate our problem
 solving abilities.  The list is not exhaustive, and is ordered by descending priority.

1. Failure to fill completely.  Originally, we intended to use the second stepper motor as a lid for the bowl, and rely 
 entirely on gravity to carry the food down the ramp and into the bowl.  While the food reached the bowl, it did not fill
 as full as we desired.  As a result, we made the compromise to replace the lid with the flap to ensure proper filling.

2. Feed screw stepper motor skipping.  Our first design did not include a gearbox on the feed screw stepper motor.  After
 constructing the feeder and testing the filling, we found that the screw stepper would occasionally skip due to excessive
 resistance from the cat food.  As a result, we had to add the gearbox to the stepper motor to increase the torque.

3. Incorrect voltage regulator feedback resistor value.  When ordering the board, I found that the online assembly service did
 not have the exact value of resistor I needed.  I picked the closest value and ordered the board.  Later, I double checked
 my work and found that the new value I picked would decrease the output voltage of the regulator to an unacceptable level.
 As a result, we had to swap out the resistor manually after the board arrived.

4. Insufficient flap motor torque.  We wanted the flap motor to be current limited such that it is incapable of harming the 
 cat.  We selected a motor with a very small output torque.  After testing, we found that the torque was insufficient to
 rotate the flap.  As a result, we had to swap out the motor to a different model.  Since the encoder was integrated, this
 also meant swapping out the encoder.  Unfortunately, the new encoder was 5V rather than 3.3V.  Luckily, we were able to 
 pull 5V from the ADC circuit to power the encoder, since the pins on the MCU that were reading the encoder were 5V 
 tolerant.

5. HTTP server development.  Unforunately, developing an http server from scratch is challenging because it requires 
 interaction with modern web browsers.  Modern web browsers have various requirements about how http servers need to 
 behave.  Further, these requirements are not well documented and web browsers provide very little error feedback.
 As a result, developing the server required a lot of guess and check before our browsers would accept the website.
 Beyond just checking the website, the UART connection used for setting up the server and website frequently drops
 characters, making the website setup inconsistent. 

6. Load cell drift.  Our initial goal with this project was to use the load cells to track how much food the cat eats,
 and provide the user with the option to regulate food and view how much has been eaten.  Unfortunately, after building
 the feeder we found that the load cells tend to drift over time.  As a result, we have no method of detecting when food
 is being eaten, because we cannot distinguish load cell drift from actual food removal.  It is therefore up to the user
 to determine how often and how much to feed so the cat has enough food, but the bowl doesn't overflow.  Future work will
 likely include mounting a webcam somewhere on the feeder so the user can view the bowl remotely.

7. Load cell filtering.  We attempted to apply a finite impulse response filter to the ADC output, but ultimately decided
 against it.  We found that the noise from the ADC was infrequent but yielded extremely high values.  As a result,
 the filter output was more a measure of how many noisy inputs were received in the given time period than an actual measure
 of the weight.  As a result, we moved toward a more simple filter that simply compares each value to the last value and
 discards it if the difference exceeds a threshold.

8. Calendar IC Alarms. Firstly, we accidentally overlooked connecting the interrupts pins from the calendar IC to the MCU,
 so currently we handle alarms using polling. In addition, receiving the current date and time from the http server proved very
 buggy when testing the website, so the user currently plans the feeding by inputting the desired feed time relative to the
 current time. Future development on the website will add absolute time alarms. 

 <pre>



</pre>

### Bill of Materials

<img src="https://cal-poly-me507-mecha07.github.io/Feeder%20BOM.jpg" style="width: 50%; height: 50%">

Figure 16.  Bill of Materials for the electrical and mechanical assemblies.

 <pre>



</pre>
