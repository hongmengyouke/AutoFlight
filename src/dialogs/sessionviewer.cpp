#include "sessionviewer.h"
#include "../autoflight.h"
#include <QtWidgets>

#include <vector>
#include <string>
#include <iostream>
#include <locale>

using namespace std;

SessionViewer::SessionViewer(QWidget *parent) : QMainWindow(parent)
{
	setWindowTitle(tr("Session Viewer"));

	tabs = new QTabWidget();

	QWidget *chooserw = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	chooserw->setLayout(layout);

	tabs->addTab(chooserw, tr("Session Chooser"));

	setCentralWidget(tabs);

	QLabel *l = new QLabel(tr("Choose the session to open"));
	l->setAlignment(Qt::AlignCenter);
	l->setStyleSheet("font-size: 25px; padding: 10px;");
	layout->addWidget(l);

	// Get the time data of the saved files
	vector<string> sessions = SessionReader::getSessionSaves();
	sort(sessions.begin(), sessions.end());

	vector<string> months;

	for(string filename : sessions)
	{
		if(filename.length() == 22)
		{
			string year = filename.substr(3, 4);
			string month = filename.substr(7, 2) + "/" + year;

			if(months.empty())
			{
				months.push_back(month);
			}
			else if(months.back() != month)
			{
				months.push_back(month);
			}
		}
	}

	// Session chooser labels
	QWidget *sessionchooserlabels = new QWidget();
	QHBoxLayout *scllayout = new QHBoxLayout();
	scllayout->setSpacing(0);
	scllayout->setContentsMargins(0, 0, 0, 0);
	sessionchooserlabels->setLayout(scllayout);
	layout->addWidget(sessionchooserlabels);

	QLabel *monthL = new QLabel(tr("Month"));
	QLabel *dayL = new QLabel(tr("Day"));
	QLabel *timeL = new QLabel(tr("Time"));

	monthL->setAlignment(Qt::AlignCenter);
	dayL->setAlignment(Qt::AlignCenter);
	timeL->setAlignment(Qt::AlignCenter);

	scllayout->addWidget(monthL);
	scllayout->addWidget(dayL);
	scllayout->addWidget(timeL);

	// Session chooser
	QWidget *sessionchooser = new QWidget();
	QHBoxLayout *sclayout = new QHBoxLayout();
	sclayout->setSpacing(0);
	sclayout->setContentsMargins(0, 0, 0, 0);
	sessionchooser->setLayout(sclayout);
	layout->addWidget(sessionchooser);

	monthchooser = new QListWidget();
	for(string month : months)
	{
		monthchooser->addItem(QString::fromStdString(month));
	}
	sclayout->addWidget(monthchooser);

	daychooser = new QListWidget();
	sclayout->addWidget(daychooser);

	timechooser = new QListWidget();
	sclayout->addWidget(timechooser);

	ok = new QPushButton(tr("Open Session"));
	ok->setEnabled(false);
	ok->setStyleSheet("font-size: 16px;");
	layout->addWidget(ok);


	QObject::connect(monthchooser, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(monthSelectionChanged(QListWidgetItem *, QListWidgetItem *)));
	if(monthchooser->count() != 0)
	{
		monthchooser->setCurrentRow(monthchooser->count() - 1);
	}

	QObject::connect(daychooser, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(daySelectionChanged(QListWidgetItem *, QListWidgetItem *)));
	QObject::connect(timechooser, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(timeSelectionChanged(QListWidgetItem *, QListWidgetItem *)));

	QObject::connect(ok, SIGNAL(clicked()), this, SLOT(loadSelectedSession()));

	///////////////////////////
    // Actual session viewer //
	///////////////////////////

	QWidget *viewer = new QWidget();
	QVBoxLayout *viewl = new QVBoxLayout();
	viewl->setSpacing(0);
	viewl->setContentsMargins(0, 0, 0, 0);
	viewer->setLayout(viewl);

	tabs->addTab(viewer, tr("Session Viewer"));

	noSessionOpen = new QLabel(tr("No session open"));
	noSessionOpen->setStyleSheet("padding: 20px; font-size: 28px;");
	noSessionOpen->setAlignment(Qt::AlignCenter);
	viewl->addWidget(noSessionOpen);

	sessionTitle = new QLabel();
	sessionTitle->setStyleSheet("padding: 10px; font-size: 26px;");
	sessionTitle->setAlignment(Qt::AlignCenter);
	sessionTitle->setMaximumHeight(50);
	viewl->addWidget(sessionTitle);
	sessionTitle->setVisible(false);

	sessionInfo = new QLabel();
	sessionInfo->setStyleSheet("padding: 5px; font-size: 18px");
	sessionInfo->setAlignment(Qt::AlignCenter);
	sessionInfo->setMaximumHeight(30);
	viewl->addWidget(sessionInfo);
	sessionInfo->setVisible(false);

	pics = new GalleryWidget(0, this);
	viewl->addWidget(pics);
	pics->setVisible(false);

	sensorviewer = new SensorDataViewer();
	viewl->addWidget(sensorviewer);

	timeline = new TimelineWidget(0);
	timeline->setMaximumHeight(100);
	timeline->setMinimumHeight(100);
	viewl->addWidget(timeline);
	timeline->setVisible(false);

	QObject::connect(timeline, SIGNAL(markerPressed(std::string, std::string, float)), this, SLOT(timelineMarkerPressed(std::string, std::string, float)));
	QObject::connect(timeline, SIGNAL(newTimeSelected(float)), this, SLOT(timelineTimeUpdated(float)));
}

void SessionViewer::monthSelectionChanged(QListWidgetItem *selected, QListWidgetItem *previous)
{
	if(selected == nullptr)
	{
		return;
	}

	vector<string> sessions = SessionReader::getSessionSaves();

	string selectedmonth = selected->text().toStdString().substr(3, 4) + selected->text().toStdString().substr(0, 2);

	vector<int> days;

	for(string session : sessions)
	{
		string month = session.substr(3, 6);

		if(month == selectedmonth)
		{
			int day = boost::lexical_cast<int>(session.substr(9, 2));
			days.push_back(day);
		}
	}

	sort(days.begin(), days.end());

	daychooser->clear();
	timechooser->clear();
	ok->setEnabled(false);

	int previousday = -1;

	for(int day : days)
	{
		if(day != previousday)
		{
			daychooser->addItem(QString::number(day));
		}

		previousday = day;
	}
}

void SessionViewer::daySelectionChanged(QListWidgetItem *selected, QListWidgetItem *previous)
{
	if(selected == nullptr)
	{
		return;
	}

	vector<string> sessions = SessionReader::getSessionSaves();

	int selectedday = boost::lexical_cast<int>(selected->text().toStdString());

	vector<string> hours;

	for(string session : sessions)
	{
		int day = boost::lexical_cast<int>(session.substr(9, 2));

		if(day == selectedday)
		{
			hours.push_back(session.substr(12, 6));
		}
	}

	sort(hours.begin(), hours.end());

	timechooser->clear();
	ok->setEnabled(false);

	for(string hour : hours)
	{
		string formatted = hour.substr(0, 2) + ":" + hour.substr(2, 2) + ":" + hour.substr(4, 2);
		timechooser->addItem(QString::fromStdString(formatted));
	}
}

void SessionViewer::timeSelectionChanged(QListWidgetItem *selected, QListWidgetItem *previous)
{
	if(selected == nullptr)
	{
		return;
	}

	ok->setEnabled(true);
}

void SessionViewer::loadSelectedSession()
{
	string filename = "AF_";
	// Append the year and month
	filename.append(monthchooser->currentItem()->text().toStdString().substr(3, 4) + monthchooser->currentItem()->text().toStdString().substr(0, 2));
	// Append the day
	int day = boost::lexical_cast<int>(daychooser->currentItem()->text().toStdString());
	if(day <= 9)
	{
		filename.append("0" + to_string(day));
	}
	else
	{
		filename.append("" + to_string(day));
	}
	// Append the time
	filename.append("T");
	string time = timechooser->currentItem()->text().toStdString();
	time.erase(remove_if(time.begin(), time.end(), not1(ptr_fun(::isdigit))), time.end());
	filename.append(time);
	// Append the extension
	filename.append(".xml");

	// Open the session
	if(_reader.readSession(AutoFlight::getHomeDirectory() + "AutoFlightSaves/Sessions/" + filename))
	{
		cout << "Opened session " << filename << endl;
		vector<RecordedEvent> events = _reader.getEvents();

		timeline->removeEvents();
		timeline->setTime((float) events.back().getTimeFromStart() / 1000.0f);

		for(RecordedEvent event : events)
		{
			timeline->addEvent(event);
		}

		openedSession();
	}
	else
	{
		cerr << "Could not open session " << filename << endl;
	}
}

void SessionViewer::openedSession()
{
	noSessionOpen->setVisible(false);

	timeline->setVisible(true);
	sessionTitle->setText(QString(daychooser->currentItem()->text()).append("/").append(monthchooser->currentItem()->text()).append(" ").append(timechooser->currentItem()->text()));
	sessionTitle->setVisible(true);
	sessionInfo->setText(tr("Session duration: ").append(QString::number(_reader.getSessionDuration(), 'f', 1)).append(tr(" | Total flight time: ")).append(QString::number(_reader.getFlightTime(), 'f', 1)).append(tr(" | Pictures taken: ")).append(QString::number(_reader.getPicturesCount(), 'f', 0)));
	sessionInfo->setVisible(true);
	pics->setImages(_reader.getPicturePaths());
	pics->setVisible(true);

	tabs->setCurrentIndex(1);
}

void SessionViewer::timelineMarkerPressed(string type, string content, float time)
{
	if(type == "PictureTaken")
	{
		pics->showImage(content);
	}
}

void SessionViewer::timelineTimeUpdated(float newTime)
{

}
