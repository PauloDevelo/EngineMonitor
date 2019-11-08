# EngineMonitor
Arduino project that allows to monitor a diesel engine on a sailboat.

Added the 28/03/2016 :
After almost 2 years I'm using this system on board, this is what it improved onboard :

-I have constantly a pretty good estimation of the quantity of diesel onboard. However, it is necessary to readjust the estimated quantity given by the engine monitor (via its configuration page) time to time. The quantity estimated is always pessimistic, so I never had bad surprise ... However, I realized that bad suprise could happen. For example if a line get stuck on your propeller, this will make your engine consuming more diesel than it should be, therefor, the diesel consumption given by this system might be wrong...

-I feel more confortable to know the engine monitor monitors the temperature by the exhaust. I will be alerted immediatly if somethings gets wrong with the salt water cooling system ...

-The maintenance of the engine is now straighforward, because the engine monitor supply the engine age throw Bluetooth to the software I developped in Java to manage the engine maintenance.

Edit from the 09/11/2019
It is still working reliably. I think, since the last edit the 28/03/2016, I added a temperature sensor in the cooling system: So now 2 temperature sensor, one on the engine coolant, and another one on the exhaust.

I re-implemented the software that manages the engine maintenance. It is now a web application (progressive). It means you can input all your maintenance engine tasks into this web application, and record all your maintenance in it. Because it is a web application progressive, you need internet the first time you visit the application, but for the next time, the application can work offline (it happens that cruisers do the engine maintenance in heaven places where there is no internet ...).

In the future, the Arduino system described here will be able to automatically update the engine age in the web application in order to remind you what are the engine maintenance tasks to do. For now, the engine age has to be edited manually throw the web application.
This web application is accessible here : https://maintenance.ecogium.fr/
