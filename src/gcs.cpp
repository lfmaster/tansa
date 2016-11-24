#include <tansa/action.h>
#include <tansa/control.h>
#include <tansa/core.h>
#include <tansa/jocsParser.h>
#include <tansa/config.h>
#include <tansa/jocsPlayer.h>
#include <tansa/mocap.h>
#include <tansa/gazebo.h>
#include <tansa/osc.h>

#ifdef  __linux__
#include <sys/signal.h>
#endif
//TODO check if these work on OSX
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

using namespace std;
using namespace tansa;

// TODO: Resolve this to an absolute path
static const char *paramsDir = "./config/params/";

static bool running;
static bool killmode = false;
static bool pauseMode = false;
static bool stopMode = false;
static bool playMode = false;
static bool prepareMode = false;
static JocsPlayer* player;
static bool useMocap;
static vector<Vehicle *> vehicles;
static std::vector<vehicle_config> vconfigs;
static vector<unsigned> jocsActiveIds;

void signal_sigint(int s) {
	// TODO: Prevent
	running = false;
}

/* For sending a system state update to the gui */
void send_status_message() {

	json j;

	j["type"] = "status";
	j["time"] = player->currentTime();

	json vehs = json::array();
	for(int i = 0; i < vehicles.size(); i++) {
		json v;
		v["id"] = vconfigs[i].net_id;
		v["role"] = i <= jocsActiveIds.size() - 1? jocsActiveIds[i] : -1;
		v["connected"] = vehicles[i]->connected;
		v["armed"] = vehicles[i]->armed;
		v["tracking"] = vehicles[i]->tracking;

		json pos = json::array();
		pos.push_back(vehicles[i]->state.position.x());
		pos.push_back(vehicles[i]->state.position.y());
		pos.push_back(vehicles[i]->state.position.z());
		v["position"] = pos;

		json bat = {
			{"voltage", vehicles[i]->battery.voltage},
			{"percent", vehicles[i]->battery.percent}
		};
		v["battery"] = bat;

		vehs.push_back(v);
	}
	j["vehicles"] = vehs;


	json global;
	global["playing"] = player->isPlaying();
	global["ready"] = player->isReady();
	j["global"] = global;

	tansa::send_message(j);
}	

void send_file_list() {
	json j;

	j["type"] = "list_reply";
	json files = json::array();
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir ("data")) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			if(ent->d_type == DT_REG && std::string(ent->d_name).find(".jocs") != std::string::npos)	
				files.push_back(std::string(ent->d_name));
		}
		closedir (dir);
	} else {
		/* could not open directory */
		//TODO Do something intelligent here
	}
	j["files"] = files;
	tansa::send_message(j);
}


void socket_on_message(const json &data) {

	string type = data["type"];

	if(type == "prepare") {
		printf("Preparing...\n");
		prepareMode = true;
	} else if(type == "play") {
		printf("Playing...\n");
		playMode = true;
	} else if(type == "land") {
		player->land();
	} else if (type == "pause") {
		printf("Pausing...\n");
		pauseMode = true;
	} else if (type == "stop") {
		printf("Stopping...\n");
		stopMode = true;
	} else if (type == "reset") {
		printf("Resetting...\n");
	} else if (type == "list"){
		printf("Sending file list...\n");
		send_file_list();
	} else if (type == "load"){
		printf("Loading jocs file...\n");
	} else if(type == "kill") {
		bool enabled = data["enabled"];
		printf("Killing...\n");
		killmode = enabled;
	} else {
		// TODO: Send an error message back to the browser
		printf("Unexpected message type recieved!\n");
	}
}


void osc_on_message(OSCMessage &msg) {

	// Address will look something like: '/cue/0101/start'
	if(msg.address[0] == "cue") {

		int num = std::stoi(msg.address[1]);

		if(msg.address[2] == "load") {
			player->prepare();
		}
		else if(msg.address[2] == "start") {
			printf("Starting at cue #: %d\n", num);

			// Assert that it is already prepared at the given cue

			player->play();
		}

	}

	/*
	printf("Address:\n");
	for(auto str : msg.address)
		printf("- %s\n", str.c_str());

	printf("\nArgs:\n");
	for(auto str : msg.args)
		printf("- %s\n", str.c_str());
	*/

}

/*
	Must be called when holding in singleDrone.jocs
*/
void do_calibrate() {
	if(vehicles.size() != 1) {
		cout << "Can only calibrate one vehicle at a time" << endl;
		return;
	}

	if(!player->isReady()) {
		cout << "Must be holding to start calibration" << endl;
		return;
	}

	cout << "Calibrating..." << endl;

	double sum = 0.0;
	Rate r(10);
	for(int i = 0; i < 40; i++) {
		sum += vehicles[0]->lastRawControlInput.z();
	}

	sum /= 40.0;

	string calibId = useMocap? to_string(vconfigs[0].net_id) : "sim";
	vehicles[0]->params.hoverPoint = sum;
	vehicles[0]->writeParams(string(paramsDir) + calibId + ".calib.json");

	cout << "Done!" << endl;
}

pthread_t console_handle;

void *console_thread(void *arg) {

	while(running) {

		// Read a command
		cout << "> ";
		string line;
		getline(cin, line);

		// Split into arguments
		vector<string> args;
		istringstream iss(line);
		while(!iss.eof()) {
			string a;
			iss >> a;

			if(iss.fail())
				break;

			args.push_back(a);
		}


		if(args.size() == 0)
			continue;



		if (args[0] == "prepare") {
			cout << "Preparing..." << endl;
			prepareMode = true;
		} else if (args[0] == "play") {
			cout << "Playing..." << endl;
			playMode = true;
		} else if (args[0] == "pause") {
			cout << "Pausing..." << endl;
			pauseMode = true;
		} else if (args[0] == "stop") {
			cout << "Stopping..." << endl;
			stopMode = true;
		} else if (args[0] == "land") {
			cout << "Landing..." << endl;
			player->land();
		} else if (args[0] == "kill") {
			killmode = args.size() <= 1 || !(args[1] == "off");
		}
		else if(args[0] == "calibrate") {
			do_calibrate();
		}


	}

}

void console_start() {
	pthread_create(&console_handle, NULL, console_thread, NULL);
}


int main(int argc, char *argv[]) {

	assert(argc == 2);
	string configPath = argv[1];

	ifstream configStream(configPath);
	if (!configStream) throw "Unable to read config file!";

	/// Parse the config file
	std::string configData((std::istreambuf_iterator<char>(configStream)), std::istreambuf_iterator<char>());
	nlohmann::json rawJson = nlohmann::json::parse(configData);
	hardware_config config;
	string jocsPath = rawJson["jocsPath"];
	vector<unsigned> activeids = rawJson["jocsActiveIds"];
	jocsActiveIds = activeids;
	useMocap = rawJson["useMocap"];
	float scale = rawJson["theaterScale"];
	bool enableMessaging = rawJson["enableMessaging"];
	bool enableOSC = rawJson["enableOSC"];

	if (useMocap) {
		nlohmann::json hardwareConfig = rawJson["hardwareConfig"];
		config.clientAddress = hardwareConfig["clientAddress"];
		config.serverAddress = hardwareConfig["serverAddress"];
	}

	vconfigs.resize(rawJson["vehicles"].size());
	for(unsigned i = 0; i < rawJson["vehicles"].size(); i++) {
		vconfigs[i].net_id = rawJson["vehicles"][i]["net_id"];
		if(useMocap) {
			vconfigs[i].lport = 14550 + 10*vconfigs[i].net_id;
			vconfigs[i].rport = 14555;
		}
		else { // The simulated ones are zero-indexed and are always in ascending order
			vconfigs[i].net_id = i;

			vconfigs[i].lport = 14550 + 10*vconfigs[i].net_id;
			vconfigs[i].rport = 14555 + 10*vconfigs[i].net_id;
		}
	}

	Jocs *jocs = Jocs::Parse(jocsPath, scale);

	auto homes = jocs->GetHomes();

	// Only pay attention to homes of active drones
	std::vector<Point> spawns;
	for (int i = 0; i < jocsActiveIds.size(); i++) {
		int chosenId = jocsActiveIds[i];
		// We assume the user only configured for valid IDs..
		spawns.push_back(homes[chosenId]);
		spawns[i].z() = 0;
	}

	tansa::init(enableMessaging);

	if(enableMessaging) {
		tansa::on_message(socket_on_message);
	}


	Mocap *mocap = nullptr;
	GazeboConnector *gazebo = nullptr;

	// Only pay attention to homes of active drones
	// TODO: Have a better check for mocap initialization/health
	if (useMocap) {
		mocap = new Mocap();
		mocap->connect(config.clientAddress, config.serverAddress);
	} else {
		gazebo = new GazeboConnector();
		gazebo->connect();
		gazebo->spawn(spawns);
	}

	int n = spawns.size();

	if (n > vconfigs.size()) {
		printf("Not enough drones on the network\n");
		return 1;
	}

	vehicles.resize(n);

	for(int i = 0; i < n; i++) {
		const vehicle_config &v = vconfigs[i];

		vehicles[i] = new Vehicle();

		// Load default parameters
		vehicles[i]->readParams(string(paramsDir) + "default.json");
		string calibId = useMocap? to_string(v.net_id) : "sim";

		if(!vehicles[i]->readParams(string(paramsDir) + to_string(v.net_id) + ".calib.json")) {
			cout << "#" + to_string(v.net_id) + " not calibrated!" << endl;
		}

		vehicles[i]->connect(v.lport, v.rport);
		if (useMocap) {
			mocap->track(vehicles[i], i+1);
		} else {
			gazebo->track(vehicles[i], i);
		}
	}

	if(enableOSC) {
		OSC *osc = new OSC();
		osc->start(53100);
		osc->set_listener(osc_on_message);
	}




	player = new JocsPlayer(vehicles, jocsActiveIds);
	player->loadJocs(jocs);

	running = true;
	signal(SIGINT, signal_sigint);

	int i = 0;

	/*
	// For sample lighting demo
	float level = 0;
	float dl = 0.005;
	*/

	signal(SIGINT, signal_sigint);
	printf("running...\n");
	running = true;

	console_start();

	Rate r(100);
	while(running) {

		// Regular status messages
		if(enableMessaging && i % 20 == 0) {
			send_status_message();
		}

		if (killmode) {
			for(Vehicle *v : vehicles)
				v->terminate();
		} else if (prepareMode) {
			prepareMode = false;
			player->prepare();
		} else if (playMode) {
			playMode = false;
			player->play();
		} else if (pauseMode) {
			pauseMode = false;
			player->pause();
		} else if (stopMode) {
			stopMode = false;
			player->stop();
		} else {
			player->step();
		}

		r.sleep();
		i++;
	}

	/// Cleanup
	if (useMocap) {
		mocap->disconnect();
		delete mocap;
	} else {
		gazebo->disconnect();
		delete gazebo;
	}

	// Stop all vehicles
	for(int vi = 0; vi < n; vi++) {
		Vehicle *v = vehicles[vi];
		v->disconnect();
		delete v;
	}

	player->cleanup();

	printf("Done!\n");
}
