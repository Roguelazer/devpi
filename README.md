/dev/pi
=======

Have you ever been sitting at your computer, doing something interesting
with Linux, and been wishing that you had more sources of random-looking (but
not actually random) data for your program? /dev/zero is fast, but it's so...
predictable. /dev/urandom has lots of entropy, but meh.

Enter */dev/pi*. It's irrationally better.

Building
--------
You'll need kernel sources and a reasonably-modern (2.6 or 3.0). Just type
`make`, then `insmod devpi.ko`. This will create a device node at `/dev/pi`,
which is only accessible by root. Feel free to use udev or whatever else
to change its permissions.

Contributing
------------
This is a Hackathon project from [yelp](http://www.yelp.com/careers). Probably
best to leave it alone for the time being. We'll update this section Soon&trade;.

License
-------
This code is license under the GNU General Public License, Version 2. The
full text of this license is available under LICENSE.txt

Authors
-------
* James Brown <mailto:jbrown@yelp.com>
* Evan Klitzke <mailto:evan@yelp.com>
