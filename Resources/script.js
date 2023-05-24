function ShowMessage(messageObj) {
    document.getElementById("result").innerHTML = JSON.stringify(messageObj, null, 2);
    // document.getElementById("result").innerHTML = message + " " + m2 + " " + m3;
}

function GainUpdate(value){
    // Rotate the gain knob
    // Map value from 0-1 to -45 to 45
    value = value * 90 - 45;
    document.getElementById("gainKnob").style.transform = "rotate(" + value + "deg)";
}

window.addEventListener('DOMContentLoaded', (event) => {


    // const button = document.getElementById('myButton');
    // button.addEventListener('click', buttonClick);
    //
    // let boxVisible = true;
    // function buttonClick() {
    //     boxVisible = !boxVisible;
    //     document.getElementById("box").style.visibility = boxVisible ? 'visible' : 'hidden';
    //
    //
    // }

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
            knob.style.transform = 'rotate(' + angle + 'deg)';
        }
    });

    // Event listener for mouse up event
    document.addEventListener('mouseup', function () {
        isDragging = false;
        // Remove dragging class from the knob
        knob.classList.remove('dragging');
    });

});