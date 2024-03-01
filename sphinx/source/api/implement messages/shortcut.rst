.. _API ISB:

ISOBUS Shortcut Button (ISB) API
================================

This is an interface for communicating as or from an ISOBUS shortcut button (ISB).
This functionality is defined in AEF Guideline 004 - ISB and at https://www.isobus.net (ISO 11783-7).

You can choose to either receive this message, send it, or both. An ISB is essentially
a command to all implements to enter a safe state. See the descriptions located at
https://www.isobus.net/isobus/pGNAndSPN/?type=PGN by searching "All implements stop operations switch state", ISO 11783-7, or
https://www.aef-online.org/fileadmin/user_upload/Content/pdfs/AEF_One_Pager.pdf
for more details.

.. warning::
    If you consume this message, you **MUST** implement an associated alarm in your
    VT object pool, along with an icon or other indication on your home screen that your
    working set master supports ISB, as required for AEF conformance.

.. doxygenclass:: isobus::ShortcutButtonInterface
   :members:
