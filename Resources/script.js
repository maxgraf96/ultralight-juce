const parser = new DOMParser();
let APVTS = null;

/**
 * Called by JUCE whenever there is a change in the APVTS parameters
 * @param xml The XML string containing the APVTS parameters. You can parse this and update your UI accordingly as below,
 * or write some more advanced logic to do more complex state management for your UI elements.
 * @constructor
 */
function APVTSUpdate(xml) {
    // Parse the XML string into an XML document
    const xmlDoc = parser.parseFromString(xml, "text/xml");
    // Get all PARAM elements from the XML document
    const paramElements = xmlDoc.getElementsByTagName("PARAM");
    // Iterate through all PARAM elements
    for (let i = 0; i < paramElements.length; i++) {
        const paramElement = paramElements[i];
        const id = paramElement.getAttribute("id");
        const value = paramElement.getAttribute("value");

        // Match all relevant ids from the APVTS XML to their UI elements
        switch (id) {
            case "gain":
                // Call the GainUpdate function with the value
                GainUpdate(value);
                break;
            // Add more cases for other relevant ids and their corresponding UI elements
            // case "otherId":
            //   OtherUpdate(value);
            //   break;
            default:
                // Handle unknown ids or do nothing
                break;
        }
    }
}

function GainUpdate(value){
    // Rotate the gain knob
    // Map value from 0-1 to -145 to 145
    value = value * 290 - 145;
    var knob = document.querySelector('#gain svg');
    knob.style.transform = "rotate(" + value + "deg)";
}

window.addEventListener('DOMContentLoaded', (event) => {
    // Get the knob element
    var knob = document.querySelector('#gain svg');

    // Variables to track mouse movement
    var isDragging = false;
    var startPosX, startPosY;
    var startAngle = 0;

    // Function to calculate the angle based on mouse movement
    function calculateAngle(posX, posY) {
        var deltaY = posY - startPosY;
        var angle = startAngle - deltaY;
        var maxAngle = 145;
        if(angle < -maxAngle)
            angle = -maxAngle;
        if(angle > maxAngle)
            angle = maxAngle;
        return angle;
    }

    // Event listener for mouse down event
    knob.addEventListener('mousedown', function (event) {
        isDragging = true;
        // Add dragging class to the knob
        knob.classList.add('dragging');
        startPosX = event.clientX;
        startPosY = event.clientY;
        startAngle = parseFloat(knob.style.transform.replace('rotate(', '').replace('deg)', '')) || 0;
    });

    // Event listener for mouse move event
    document.addEventListener('mousemove', function (event) {
        if (isDragging) {
            var angle = calculateAngle(event.clientX, event.clientY);
            // Update the gain value in JUCE
            let juceGain = (angle + 145) / 290;
            OnParameterUpdate("gain", juceGain);
            // Using jQuery
            $('#gain svg').css('transform', 'rotate(' + angle + 'deg)');
        }
    });

    // Event listener for mouse up event
    document.addEventListener('mouseup', function () {
        isDragging = false;
        // Remove dragging class from the knob
        knob.classList.remove('dragging');
    });

});