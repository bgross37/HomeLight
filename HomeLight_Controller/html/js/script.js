class DeviceState {
    mode = '#';
    values = [0, 0, 0, 0];
    ws = undefined;
    
    constructor(mode, values, ws){
        this.mode = mode
        this.values = values
        this.ws = ws;
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
        this.ws.send(message);
    }

}



// ----- INITIALIZATION STEPS
const DEVICETYPES = readXML('DeviceTypes.xml').devicetypes.device
let DEVICES = readXML('Devices.xml').devices.device;
let errorcount = 0;


//xml parser returns just an object of only one tag is found. Needs to be an array all the time.
if(!Array.isArray(DEVICES)){
    DEVICES = [DEVICES];
}

//iterate over the objects in the list
Object.entries(DEVICES).forEach(([key, device]) => {
    device.isReady = false;
    let ws = new WebSocket("ws://" + device._mdns + ":81")
    ws.addEventListener('open', function (event) {
        ws.send('$'); //request current status
        buildHTML(device);
    });
    
    ws.addEventListener('error', function(event){
        errorcount++;
        device.isError = true;
        buildError(device);
        if(key == DEVICES.length - errorcount - 1){
            activateMainButton();
        }
    });
    
    ws.addEventListener('message', function (event) {
        //the first response will have the current status - this will be used to create the object
        if(!event.data.startsWith('R')){
            alert('Response from ' + event.origin + ' was invalid. Response: ' + event.data);
            device.isError = true;
        }
        else{
            device.isError = false;
            ws.addEventListener('close', function (event) {
                window.location.reload(false);
            });
        }
        let response = parseResponseString(event.data);
        if(!device.isReady){
            device.deviceType = response.deviceType;
            device.sliderConfig = DEVICETYPES[response.devicetype].mode.find(x => x._id === response.mode).control
            device.state = new DeviceState(response.mode, response.values, ws);
            
            createControls(device);
            
            if(key == DEVICES.length - errorcount - 1){
                activateMainButton();
            }
            device.isReady = true;
        }
        else{
            device.sliderObjects[0].value = response.values[0];
            device.sliderObjects[1].value = response.values[1];
            device.sliderObjects[2].value = response.values[2];
            device.sliderObjects[3].value = response.values[3];
        }
    });
})



function activateMainButton(){
    document.getElementById('mainButtonContainer').classList.remove('hidden');
}

function mainButtonClicked(item){
    value = item.value;
    Object.entries(DEVICES).forEach(([key, device]) => {
        if(!device.isError){
            device.state.receiveColors(device.state.values[0], device.state.values[1], value, value);
            device.state.ws.send('$');
        }
    })
}


//build the HTML. This is called after all devices have been created and received their statuses.
function buildHTML(device){
    let lightBox = document.createElement('div');
    lightBox.classList.add('lightBox');
    
    let lightIcon = document.createElement('img');
    lightIcon.classList.add('lightIcon');
    lightIcon.src = device._icon;
    
    let lightTitle = document.createElement('h4');
    lightTitle.classList.add('lightTitle');
    lightTitle.innerHTML = device._name;
    
    let colorBox = document.createElement('div');
    colorBox.classList.add('colorBox')
    colorBox.id = "colorBox_" + device._mdns;
    
    let lightControlContainer = document.createElement('div');
    lightControlContainer.classList.add('lightControlContainer');
    lightControlContainer.classList.add('row2');
    lightControlContainer.id = 'controlContainer_' + device._mdns;
    
    lightBox.appendChild(lightIcon);
    lightBox.appendChild(lightTitle);
    lightBox.appendChild(colorBox);
    lightBox.appendChild(lightControlContainer);
    
    document.getElementById('lightsContainer').appendChild(lightBox);
}

function createControls(device){
    document.getElementById('controlContainer_' + device._mdns).appendChild(createHSVSliders(document.getElementById('colorBox_' + device._mdns), device.state.receiveColors.bind(device.state), device.state.values, device.sliderConfig, device.sliderObjects = []));
}

//building an error header
function buildError(device){
    if(device.isError){
        console.log("Error on this device: ")
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
    Object.entries(DEVICES).forEach(([key, device]) => {
        device.state.ws.close();
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
function createHSVSliders(colorBox, callback, values, config, sliderObjects){
    let hue = document.createElement('input');
    let sat = document.createElement('input');
    let val = document.createElement('input');
    let white = document.createElement('input');
    
    sliderObjects.push(hue);
    sliderObjects.push(sat);
    sliderObjects.push(val);
    sliderObjects.push(white);
    
    console.log(values)

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
        sat.style.background = "-webkit-gradient(linear, left top, right top, from(rgb(199, 199, 199) to(hsl(" + (hue.value * 1.41) + ", 100%, 50%)))";
        //sat.style.background = "linear-gradient(to right, rgb(199, 199, 199) 0%, hsl(" + (hue.value * 1.41) + ", 100%, 50%) 100%)";
        
        val.style.background = "-webkit-gradient(linear, left top, right top, from(#000000) to(hsl(" + (hue.value * 1.41) + ", " + (sat.value / 2.55) + "%, 50%)))";
        //val.style.background = "linear-gradient(to right, #000000 0%, hsl(" + (hue.value * 1.41) + ", " + (sat.value / 2.55) + "%, 50%) 100%)";

        colorBox.style.backgroundColor = calculatePreviewColor(hue.value, sat.value, val.value);
        callback(hue.value, sat.value, val.value, white.value);
    }
    sat.oninput = function(){
        val.style.background = "-webkit-gradient(linear, left top, right top, from(#000000) to(hsl(" + (hue.value * 1.41) + ", " + (sat.value / 2.55) + "%, 50%)))";
        //val.style.background = "linear-gradient(to right, #000000 0%, hsl(" + (hue.value * 1.41) + ", " + (sat.value / 2.55) + "%, 50%) 100%)";
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
    
    config[0]._active == "true" ? contHue.classList.add('visibleControl') : contHue.classList.add('hiddenControl');
    config[1]._active == "true" ? contSat.classList.add('visibleControl') : contSat.classList.add('hiddenControl');
    config[2]._active == "true" ? contVal.classList.add('visibleControl') : contVal.classList.add('hiddenControl');
    config[3]._active == "true" ? contWhite.classList.add('visibleControl') : contWhite.classList.add('hiddenControl');
    
    return container;
}


function calculatePreviewColor(hue, sat, val){
    let rgbColor = HSVtoRGB(hue, sat, val);
    return "rgb(" + rgbColor.r + "," + rgbColor.g + "," + rgbColor.b + ")";
}
