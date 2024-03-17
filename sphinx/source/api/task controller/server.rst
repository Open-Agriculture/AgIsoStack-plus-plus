.. _API ServerAPI:

Server API
==========

Overview
^^^^^^^^

Creating a task controller server which does data logging, mapping, section control, and other common ISOBUS things is a very complex task.

AgIsoStack++ provides an abstract server API to make the CAN portion of this task easier. It allows you to create a task controller server by implementing a few virtual functions, and handles most of the CAN communication for you.

Features:

- Supports the CAN messaging needed for TC-GEO, TC-SC, and TC-BAS
- Manages DDOP transfers for you, and abstracts the client connection process so that you can focus on your application logic
- Supports both version 3 and version 4 of the TC standard in ISO11783-10
- Manages the TC status message
- Tracks client connection status and timeouts
- Integrates well with our Device Descriptor Object Pool Classes for access to product and implement information
- Provides a simple interface for sending commands and receiving values from the client
- Includes some helpers for parsing a DDOP, which can tell you the implement geometry and product information quickly without needing to write a lot of code.
- Sends all the required responses to the clients on your behalf, so you don't need to populate messages yourself.

The functions you need to implement in order to get the TC server working are (in no particular order):

- :code:`activate_object_pool`
    - This will be called by the server when a valid, connected client requests that the server activate the latest DDOP it uploaded.
    - When this function is called, you should parse the DDOP, potentially using `our DDOP classes <https://github.com/Open-Agriculture/AgIsoStack-plus-plus/blob/main/isobus/include/isobus/isobus/isobus_device_descriptor_object_pool.hpp>`_. (if you have previously received one via a call to :code:`store_device_descriptor_object_pool`).
    - You should then "activate" the DDOP by doing whatever is necessary to make your application use the new DDOP (this is application specific for your task controller).
    - You should return true here if the DDOP was valid and activated, and false if it was not valid.
    - If you return false, you must populate the error parameters with a reason why the DDOP was not valid.

- :code:`change_designator`
   - This will be called by the server when a valid, connected client requests that the server change a designator for something in the DDOP.
   - This is a common way to change the language of the DDOP, rename the implement, or change the displayed product name.
   - You should implement this function to change the designator of the object and return whether or not it was successful. 

- :code:`deactivate_object_pool`
    - This will be called by the server when a valid, connected client requests that the server deactivate its currently active DDOP.
	- When this function is called, you should deactivate the DDOP by doing whatever is necessary to make your application stop using the DDOP (this is application specific for your task controller).
	- You should return true here if the DDOP was deactivated, and false if it was not valid.
	- If you return false, you must populate the error parameter with a reason why the request failed.

- :code:`get_is_stored_device_descriptor_object_pool_by_structure_label`
    - Part of the connection process with the client involves the client possibly asking the server if it already has a DDOP with a certain structure label, associated to that client's NAME.
    - This function should return true if the server has a DDOP with the given structure label for that client's NAME already stored in non-volatile memory, and false if it does not.
    - Note that the structure label will always be 7 bytes long, and the extended structure label is optional, and may be empty or up to 32 bytes long.
    - Both labels should match exactly for the function to return true.
    - If no extended label is provided, it may be ignored.
    - Note that normally a TC should only store the latest DDOP, and not multiple DDOPs with the same structure label(s).

- :code:`get_is_stored_device_descriptor_object_pool_by_localization_label`
    - Part of the connection process with the client involves the client possibly asking the server if it already has a DDOP with a certain localization label, associated to that client's NAME.
    - The localization label describes the units, language, and country of the DDOP.
    - This function should return true if the server has a DDOP with the given localization label for that client's NAME already stored in non-volatile memory, and false if it does not.
    - Note that the localization label will always be 7 bytes long.
    - Note that normally a TC should only store the latest DDOP, and not multiple DDOPs with the same localization label.

- :code:`get_is_enough_memory_available`
    - This function will be called to determine if the server has enough memory (both RAM and ROM) to store a DDOP that a client wants to transfer.
    - Generally, the server should return true if it has enough memory to store the DDOP, and false if it does not.
    - Returning a value of true indicates "There may be enough memory available. However, because there is overhead associated with object storage,it is impossible to predict whether there is enough memory available." and false indicates "There is not enough memory available. Do not transmit device descriptor object pool."

- :code:`identify_task_controller`
    - This function will be called if someone requests that the TC identify itself. If this gets called, you should display the TC number for 3 seconds if your TC has a visual interface. 

- :code:`on_client_timeout`
    - This function will be called by the server when a connected client times out. 
    - You should implement this function to do whatever you want to do when a client times out. 
    - Generally this means you will want to also deactivate the DDOP for that client. 

- :code:`on_process_data_acknowledge`
    - This function will be called by the server when a client sends an acknowledgement for a process data command that was sent to it. 
    - This can be useful to know if the client received the last command you sent to it, or not, when using the :code:`set_value_and_acknowledge` function.

- :code:`on_value_command`
    - This function will be called by the server when a client sends a value command to the TC. This is the main way the client will provide you with data!
    - You should implement this function to do whatever you want to do when a client sends a value command. This could be anything from setting a value in your program, logging the value to a file, drawing something on a map, or sending a command to a connected implement.
    - The client could be telling you that a section's state changed, or that a boom's position changed, etc. Therefore this is probably the most important function to implement to get your TC "working". Use the `ISOBUS data dictionary <https://www.isobus.net/isobus/>`_ to determine what the parameters mean.

- :code:`store_device_descriptor_object_pool`
    - This function is called when the server wants you to save a DDOP to non volatile memory (NVM).
    - You should implement this function to save the DDOP to NVM. 
    - If :code:`appendToPool` is true, you should append the DDOP to the existing DDOP in NVM. Clients may send DDOPs in may different parts so it is imperative that you handle this correctly.

API
^^^

.. doxygenclass:: isobus::TaskControllerServer
   :members:
