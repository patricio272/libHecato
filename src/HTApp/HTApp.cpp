#include <tinyxml.h>
#include "HTAppSettings.h"
#include "HTApp.h"
#include "HTContext.h"


HTApp::HTApp() : sf::RenderWindow(sf::VideoMode(1920, 480), "HTApp"),
                                   circle(15.f),
                                   rectShape(sf::Vector2f(4.f, getSize().y)),
                                   sender(0),
                                   recv(0)
{
    printf("=====\nHTAPP\n=====\n");
    sf::Vector2u winDim = getSize();
    winW = (float)(winDim.x);
    winH = (float)(winDim.y);
    if (HTAppSettings::useSender())
    {
        printf("Using sender...\n");
        sender = new HTNetworkSender();
        sender->setHostAndPort(HTAppSettings::getSenderHost(), HTAppSettings::getSenderPort());
    }
    else
    {
        printf("Sender: OFF.\n");
    }

    if (HTAppSettings::useReceiver())
    {
        printf("Using receiver...\n");
        recv = new HTNetworkReceiver(HTAppSettings::getReceiverPort());
        recv->setBlobResultTarget(this);
    }
    else
    {
        printf("Receiver: OFF.\n");
    }

    if (HTAppSettings::useTUIO())
    {
        const std::vector<HTAppSettings::TUIOServerData>& tServers = HTAppSettings::getTUIOServers();
        printf("Using TUIO... (%lu servers)\n", tServers.size());
        for (unsigned int i = 0; i < tServers.size(); i++)
        {
            HTTUIOServer* tuioServer;
            tuioServer = new HTTUIOServer();
            tuioServer->setupConnection(tServers[i].host.c_str(), tServers[i].port);
            tuioServer->setMargins(HTAppSettings::getLeftMargin(), HTAppSettings::getRightMargin());
            tuioServers.push_back(tuioServer);
        }
    }
    else
    {
        printf("TUIO: OFF.\n");
    }
    HTContext::initialize();
    const std::vector<HTDeviceThreaded*>& devs = HTContext::getDevices();
    printf("Initializing HTContext...found %lu devices.\n", devs.size());
    for (unsigned int i = 0; i < devs.size(); i++)
    {
        HTDeviceThreaded* k = devs[i];
        k->setBlobResultTarget(this);
        k->setEnablePersonTracking(HTAppSettings::usePersonTracking());
        if (sender)
            sender->registerGenerator(k, i);
    }

    setCorrelationRadiusW(HTAppSettings::getCorrelationW());
    setCorrelationRadiusH(HTAppSettings::getCorrelationH());
    setCorrelationPerson(HTAppSettings::getPersonCorrelation());

    //in this example we won't need any generation of click events, so set this to 0.f
    printf("HTApp: Setting click radius to: %f\n", HTAppSettings::getRadiusClick());
    printf("HTApp: Setting drag  radius to: %f\n", HTAppSettings::getRadiusDrag());
    setClickRadius(HTAppSettings::getRadiusClick());
    setDragRadius(HTAppSettings::getRadiusDrag());

    setTrackRecordTarget(this);
}

HTApp::~HTApp()
{
    if (sender)
        delete sender;
    if (recv)
        delete recv;

    for (unsigned int i = 0; i < tuioServers.size(); i++)
    {
        delete tuioServers[i];
    }

    HTContext::shutdown();
}

void HTApp::update()
{
    HTContext::updateAll();
}

void HTApp::lockAndDrawAll()
{
	drawMutti.lock();
	char suffix[16] = {0};
	sf::String drawInfo;
	drawInfo = "TUIO    : ";
	HTAppSettings::useTUIO() ? drawInfo += "YES\n" : drawInfo += "NO\n";
	drawInfo += "SENDER  : ";
	HTAppSettings::useSender() ? drawInfo += "YES\n" : drawInfo += "NO\n";
	drawInfo += "RECEIVER: ";
	HTAppSettings::useReceiver() ? drawInfo += "YES\n" : drawInfo += "NO\n";
	drawInfo += "PERSTRACKING: ";
	HTAppSettings::usePersonTracking() ? drawInfo += "YES\n" : drawInfo += "NO\n";
	drawInfo += "Registered Generators: ";
	sprintf(suffix, "%u", getNumGenerators());
	drawInfo += suffix;
	sf::Text infoText;
	infoText.setString(drawInfo);
	infoText.setCharacterSize(15);
	infoText.setPosition(10, 10);
	draw(infoText);
    const unsigned len = drawBlobs.size();
    for (unsigned i = 0; i < len; i++)
    {
        //CASE 1: we have a hand blob
        if (drawBlobs[i].brtype == HTIBlobResultTarget::BRT_HAND)
        {
            sprintf(suffix, "%u", (drawBlobs[i].id));
            std::string blobid = "";
            blobid += suffix;
            circle.setPosition(drawBlobs[i].vec);
            draw(circle);
            sf::String str(blobid);
            sf::Text t;
            t.setString(str);
            t.setCharacterSize(20);
            t.setPosition(drawBlobs[i].vec.x, drawBlobs[i].vec.y+30.f);
            draw(t);
        }
        //CASE 2: we have a person blob
        else
        {
            rectShape.setPosition(drawBlobs[i].vec.x, 0.f);
            draw(rectShape);
        }
    }
//    drawBlobs.clear();
}

void HTApp::unlock()
{
	drawMutti.unlock();
}

void HTApp::handleEvents(const std::vector<HTBlobInterpreter::TrackRecord>& events)
{
	drawMutti.lock();
    drawBlobs.clear();
    const unsigned len = events.size();

    for (unsigned i = 0; i < len; i++)
    {
        const HTBlobInterpreter::TrackRecord* rec = &(events[i]);
        drawBlobs.push_back(BlobDisplay(rec->curXsmooth * winW, rec->curYsmooth * winH, rec->blobID, rec->brtype));

    }
	drawMutti.unlock();

	const unsigned int tlen = tuioServers.size();
	for (unsigned int i = 0; i < tlen; i++)
    {
        tuioServers[i]->handleEvents(events);
    }
}

void HTApp::handleBlobResult(std::vector<HTIBlobResultTarget::SBlobResult>& points, int id)
{
    if (sender)
        sender->handleBlobResult(points, id);

    HTBlobInterpreterMulti::handleBlobResult(points, id);
}

void HTApp::onResize()
{
    winW = (float)(getSize().x);
    winH = (float)(getSize().y);
}


