
$(document).ready(function(){
     x = document.getElementsByClassName("card");
     for (i = 0; i < 10; i++) {
         x[i].style.display = "block";
     }
     $("#1").addClass("w3-red");
});


$(document).ready(function(){
    $(".tablink").click(function(){
        var i, x, tablinks;
        x = document.getElementsByClassName("card");
        for (i = 0; i < x.length; i++) {
            x[i].style.display = "none";
        }
        tablinks = document.getElementsByClassName("tablink");
        for (i = 0; i < tablinks.length; i++) {
            tablinks[i].className = tablinks[i].className.replace(" w3-red", "");
        }
        pagenum = parseInt($(this).attr("id"))
        for (i = (pagenum - 1) * 10 + 1; i <= pagenum * 10; i++) {
            document.getElementById("item_" + i.toString()).style.display = "block";
        }
        $(this).addClass("w3-red");
    });
});
