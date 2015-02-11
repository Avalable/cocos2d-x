package org.cocos2dx.lib;

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;

import android.text.Editable;
import android.text.TextWatcher;
import android.widget.EditText;

public class Cocos2dxCustomTextWatcher implements TextWatcher{

	private EditText editText;
	
	public Cocos2dxCustomTextWatcher(EditText editText){    
        this.editText = editText;    
    }

	@Override
	public void afterTextChanged(Editable arg0) {
		
	}

	@Override
	public void beforeTextChanged(CharSequence arg0, int arg1, int arg2,
			int arg3) {
	}

	@Override
	public void onTextChanged(CharSequence arg0, int arg1, int arg2, int arg3) {
		
		String editable = editText.getText().toString();    
        String str = stringFilter(editable.toString());  
        
        if(!editable.equals(str)){  
            editText.setText(str);      
            editText.setSelection(str.length());  
        } 
	}  
	
	public static String stringFilter(String str) throws PatternSyntaxException {       
	    //          
	    String   regEx  = "[^0-9\\-]";                       
	    Pattern   p   =   Pattern.compile(regEx);       
	    Matcher   m   =   p.matcher(str);       
	    return   m.replaceAll("").trim();       
	}   
}
