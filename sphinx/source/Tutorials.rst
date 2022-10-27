.. _Tutorials:

Tutorials
==========

.. toctree::
   :hidden:
   :glob:

Working with NAMEs
===================

In the library, you can work with NAMEs using the NAME class.

For example, if you make an empty NAME, like this:

.. code-block:: c++

   isobus::NAME testNAME(0);

You can set any of the values inside it, like this:

.. code-block:: c++

   testNAME.set_arbitrary_address_capable(true);
   testNAME.set_industry_group(1);
   testNAME.set_device_class(0);
   testNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
   testNAME.set_identity_number(2);
   testNAME.set_ecu_instance(0);
   testNAME.set_function_instance(0);
   testNAME.set_device_class_instance(0);
   testNAME.set_manufacturer_code(64);

In this example, we completely set up a NAME with all of its components.
