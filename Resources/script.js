function ShowMessage(messageObj) {
    document.getElementById("result").innerHTML = JSON.stringify(messageObj, null, 2);
    // document.getElementById("result").innerHTML = message + " " + m2 + " " + m3;
}

function GainUpdate(value){
    document.getElementById("gain").innerHTML = value.toString();
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

});