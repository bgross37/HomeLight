class Device {
    address = 0;
    devicetype = 0;
    mode = '#';
    values = [0, 0, 0, 0];
    websocket = undefined;
    sliderConfig = undefined;
    
    constructor(address, devicetype, mode, values, websocket, name, icon, sliderConfig){
        this.address = address
        this.devicetype = devicetype
        this.mode = mode
        this.values = values
        this.websocket = websocket
        this.name = name
        this.icon = icon
        this.sliderConfig = sliderConfig
    }
    
    receiveColors(hue, sat, val, white){
        this.values[0] = parseInt(hue);
        this.values[1] = parseInt(sat);
        this.values[2] = parseInt(val);
        this.values[3] = parseInt(white);
        this.sendValues();
    }
    
    sendValues(){
        let message = this.mode;
        for(let i = 0; i < 4; i++){
            if(this.values[i] < 16){
                message += '0';
                message += this.values[i].toString(16);
            }
            else{
                message += this.values[i].toString(16)
            }
        }
        this.websocket.send(message);
    }

}



// ----- INITIALIZATION STEPS
const DEVICETYPES = readXML('DeviceTypes.xml').devicetypes.device
let DEVICES = readXML('Devices.xml').devices.device;
let htmlBuildCompleted = false;
let websockets = [];
//let deviceObjects = [];
let errorcount = 0;

//xml parser returns just an object of only one tag is found. Needs to be an array all the time.
if(!Array.isArray(DEVICES)){
    DEVICES = [DEVICES];
}

//iterate over the objects in the list
Object.entries(DEVICES).forEach(([key, device]) => {
    let ws = new WebSocket("ws://" + device._mdns + ":81")
    ws.addEventListener('open', function (event) {
        ws.send('$'); //request current status
        //TODO: create the header here
    });
    ws.addEventListener('error', function(event){
        errorcount++;
        device.isError = true;
        buildError(device);
        //TODO: Modify box to say error
    });
    ws.addEventListener('message', function (event) {
        //the first response will have the current status - this will be used to create the object
        if(!event.data.startsWith('R')){
            alert('Response from ' + event.origin + ' was invalid. Response: ' + event.data);
            device.isError = true;
        }
        else{
            device.isError = false;
        }
        let response = parseResponseString(event.data);
        let deviceConfig = DEVICETYPES[response.devicetype].mode.find(x => x._id === response.mode).control
        let newDevice = new Device(device._mdns, response.devicetype, response.mode, response.values, ws, device._name, device._icon, deviceConfig);
        websockets.push(ws);
        buildHTML(newDevice);
        //TODO: only build the controls here
    });
})



//build the HTML. This is called after all devices have been created and received their statuses.
function buildHTML(device){
        let lightBox = document.createElement('div');
        lightBox.classList.add('lightBox');
        
        let lightIcon = document.createElement('img');
        lightIcon.classList.add('lightIcon');
        lightIcon.src = device.icon;
        
        let lightTitle = document.createElement('h4');
        lightTitle.classList.add('lightTitle');
        lightTitle.innerHTML = device.name;
        
        let colorBox = document.createElement('div');
        colorBox.classList.add('colorBox')
        
        let lightControlContainer = document.createElement('div');
        lightControlContainer.classList.add('lightControlContainer');
        lightControlContainer.classList.add('row2');
        lightControlContainer.appendChild(createHSVSliders(colorBox, device.receiveColors.bind(device), device.values, device.sliderConfig));
        
        lightBox.appendChild(lightIcon);
        lightBox.appendChild(lightTitle);
        lightBox.appendChild(colorBox);
        lightBox.appendChild(lightControlContainer);
        
        document.getElementById('lightsContainer').appendChild(lightBox);
}

//building an error header
function buildError(device){
    if(device.isError){
        console.log(device)
        
        let lightBox = document.createElement('div');
        lightBox.classList.add('lightBox');
        
        let lightIcon = document.createElement('img');
        lightIcon.classList.add('lightIcon');
        lightIcon.src = 'assets/error.png';
        
        let lightTitle = document.createElement('h4');
        lightTitle.classList.add('lightTitle');
        lightTitle.classList.add('errorTitle');
        lightTitle.innerHTML = device._name;
        
        lightBox.appendChild(lightIcon);
        lightBox.appendChild(lightTitle);
        
        document.getElementById('lightsContainer').appendChild(lightBox);
    }
}


//close websockets when unloading
window.onbeforeunload = function(){
    websockets.forEach(function(ws){
        ws.close();
    })
}


//parsing the response from the ESP
function parseResponseString(response){
    let settings = {};
    settings.devicetype = response.charAt('1');
    settings.mode = response.charAt('2');
    settings.values = [];
    let values = response.substring(3).split('');
    for(let i = 0; i < values.length; i += 2){
        settings.values.push(parseInt(Number("0x" + values[i] + values[i+1])));
    }
    return settings;
}



/**
 * Builds HTML sliders
 */
function createHSVSliders(colorBox, callback, values, config){
    let hue = document.createElement('input');
    let sat = document.createElement('input');
    let val = document.createElement('input');
    let white = document.createElement('input');

    hue.type = 'range';
    hue.min = '0';
    hue.max = '255';
    config[0]._active == "true" ? hue.value = values[0] : hue.value = config[0]._inactiveValue;
    
    sat.type = 'range';
    sat.min = '0';
    sat.max = '255';
    config[1]._active == "true" ? sat.value = values[1] : sat.value = config[1]._inactiveValue;
    
    val.type = 'range';
    val.min = '0';
    val.max = '255';
    config[2]._active == "true" ? val.value = values[2] : val.value = config[2]._inactiveValue;
    
    white.type = 'range';
    white.min = '0';
    white.max = '255';
    config[3]._active == "true" ? white.value = values[3] : white.value = config[3]._inactiveValue;
    
    classAdder(hue, ['slider', 'slider-hue'])
    classAdder(sat, ['slider'])
    classAdder(val, ['slider'])
    classAdder(white, ['slider', 'slider-white'])
    
    
    hue.oninput = function(){
        sat.style.background = "linear-gradient(to right, rgb(199, 199, 199) 0%, hsl(" + (hue.value * 1.41) + ", 100%, 50%) 100%)";
        val.style.background = "linear-gradient(to right, #000000 0%, hsl(" + (hue.value * 1.41) + ", " + (sat.value / 2.55) + "%, 50%) 100%)";
        colorBox.style.backgroundColor = calculatePreviewColor(hue.value, sat.value, val.value);
        callback(hue.value, sat.value, val.value, white.value);
    }
    sat.oninput = function(){
        val.style.background = "linear-gradient(to right, #000000 0%, hsl(" + (hue.value * 1.41) + ", " + (sat.value / 2.55) + "%, 50%) 100%)";
        colorBox.style.backgroundColor = calculatePreviewColor(hue.value, sat.value, val.value);
        callback(hue.value, sat.value, val.value, white.value);
    }

    val.oninput = function(){
        colorBox.style.backgroundColor = calculatePreviewColor(hue.value, sat.value, val.value);
        callback(hue.value, sat.value, val.value, white.value);
    }
    
    white.oninput = function(){
        callback(hue.value, sat.value, val.value, white.value);
    }
    
    sat.style.background = "linear-gradient(to right, rgb(199, 199, 199) 0%, hsl(" + (values[0] * 1.41) + ", 100%, 50%) 100%)";
    val.style.background = "linear-gradient(to right, #000000 0%, hsl(" + (values[0] * 1.41) + ", " + (values[1] / 2.55) + "%, 50%) 100%)";
    colorBox.style.backgroundColor = calculatePreviewColor(values[0], values[1], values[2]);
    
    let container = document.createElement('div');
    
    let contHue = document.createElement('div');
    let contSat = document.createElement('div');
    let contVal = document.createElement('div');
    let contWhite = document.createElement('div');
    
    contHue.classList.add('sliderContainer');
    contSat.classList.add('sliderContainer');
    contVal.classList.add('sliderContainer');
    contWhite.classList.add('sliderContainer');
    
    contHue.appendChild(hue);
    contSat.appendChild(sat);
    contVal.appendChild(val);
    contWhite.appendChild(white);
    
    container.appendChild(contHue);
    container.appendChild(contSat);
    container.appendChild(contVal);
    container.appendChild(contWhite);
    
    config[0]._active == "true" ? contHue.classList.add('shown') : contHue.classList.add('hidden');
    config[1]._active == "true" ? contSat.classList.add('shown') : contSat.classList.add('hidden');
    config[2]._active == "true" ? contVal.classList.add('shown') : contVal.classList.add('hidden');
    config[3]._active == "true" ? contWhite.classList.add('shown') : contWhite.classList.add('hidden');
    
    return container;
}


function calculatePreviewColor(hue, sat, val){
    let rgbColor = HSVtoRGB(hue, sat, val);
    return "rgb(" + rgbColor.r + "," + rgbColor.g + "," + rgbColor.b + ")";
}
