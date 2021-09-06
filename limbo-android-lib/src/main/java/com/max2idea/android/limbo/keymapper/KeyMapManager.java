/*
Copyright (C) Max Kastanas 2012

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
package com.max2idea.android.limbo.keymapper;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Base64;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.SimpleAdapter;

import com.limbo.emu.lib.R;
import com.max2idea.android.limbo.keyboard.KeyboardUtils;
import com.max2idea.android.limbo.main.Config;
import com.max2idea.android.limbo.main.LimboSettingsManager;
import com.max2idea.android.limbo.screen.ScreenUtils;
import com.max2idea.android.limbo.toast.ToastUtils;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Manages loading, editing, storing, and viewing the keymapper layouts
 *
 */
public class KeyMapManager {
    private static final String TAG = "KeyMapManager";

    private final Activity activity;
    private final View view;
    private final List<HashMap<String, Object>> keyMappers = new ArrayList<>();
    private final int defaultRows;
    private final int defaultCols;
    public RelativeLayout mapperEditLayout;
    public RelativeLayout mapperButtons;

    public KeySurfaceView keySurfaceView;
    public KeyMapper keyMapper;
    OnSendKeyEventListener sendKeyEventListener = null;
    OnSendMouseEventListener sendMouseEventListener = null;
    OnUnhandledTouchEventListener unhundledTouchEventListener = null;
    private ImageButton mAddKeyMapper;
    private ImageButton mRemoveKeyMapper;
    private ImageButton mAddSpecialKeysButtons;
    private ImageButton mUseKeyMapper;
    private ImageButton mClearKey;
    private ImageButton mRepeatKey;
    private ListView mKeyMapperList;
    private SimpleAdapter keyMapperAdapter;
    private EditText mKeyMapperName;
    private HashMap<String, Object> selectedMap;

    public KeyMapManager(Activity activity, View view, int rows, int cols) throws Exception {
        this.activity = activity;
        this.view = view;
        this.defaultRows = rows;
        if (cols % 2 != 0)
            throw new Exception("Cols should be even number!");
        this.defaultCols = cols;
        setupWidgets();
        setupKeyMapper();
    }

    private void setupWidgets() {
        mapperEditLayout = activity.findViewById(R.id.mapperEditLayout);
        mapperEditLayout.setVisibility(View.GONE);
        mapperButtons = activity.findViewById(R.id.mapperButtons);
        mapperButtons.setVisibility(View.GONE);
        mKeyMapperList = activity.findViewById(R.id.keyMapperList);

        mAddKeyMapper = activity.findViewById(R.id.addKeyMapper);
        mKeyMapperName = activity.findViewById(R.id.key_mapper_name);

        mRemoveKeyMapper = activity.findViewById(R.id.removeKeyMapper);
        mAddSpecialKeysButtons = activity.findViewById(R.id.addSpecialKeysButtons);
        mUseKeyMapper = activity.findViewById(R.id.useKeyMapper);
        mClearKey = activity.findViewById(R.id.clearKey);
        mRepeatKey = activity.findViewById(R.id.repeatKey);


    }

    public boolean toggleKeyMapper() {
        boolean shown = false;
        if (isEditMode() || isActive()) {
            mapperEditLayout.setVisibility(View.GONE);
            mapperButtons.setVisibility(View.GONE);
            clearKeyMapper();
            shown = false;
        } else {
            mapperEditLayout.setVisibility(View.VISIBLE);
            mapperButtons.setVisibility(View.VISIBLE);
            shown = true;
        }
        return shown;
    }

    private boolean isActive() {
        return mapperButtons != null && mapperButtons.getVisibility() == View.VISIBLE;
    }

    public boolean processKeyMap(KeyEvent event) {
        if (isEditMode() && keySurfaceView.pointers.size() > 0) {
            if (event != null) {
                keySurfaceView.setKeyCode(event);
                keySurfaceView.paint(true);
            }
            return true;
        }
        return false;
    }


    public boolean processMouseMap(int button, int action) {
        if (isEditMode()
                && keySurfaceView.pointers.size() > 0
                && (button == Config.SDL_MOUSE_LEFT || button == Config.SDL_MOUSE_MIDDLE || button == Config.SDL_MOUSE_RIGHT)
        ) {
            keySurfaceView.setMouseButton(button);
            keySurfaceView.paint(true);
            return true;
        }
        return false;
    }

    public void sendKeyEvent(int keyCode, boolean down) {
        if (sendKeyEventListener != null)
            sendKeyEventListener.onSendKeyEvent(keyCode, down);
    }

    public void sendMouseEvent(int button, boolean down) {
        if (sendMouseEventListener != null)
            sendMouseEventListener.onSendMouseEvent(button, down);
    }

    private void setupKeyMapper() throws Exception {
        setupKeyMapperButtons();
        setupKeyMapperList();
        setupKeyMapperLayout();
    }

    private void setupKeyMapperLayout() throws Exception {
        mapperButtons.removeAllViews();
        keySurfaceView = new KeySurfaceView(activity, this);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);
        mapperButtons.addView(keySurfaceView, params);
    }

    private void setupKeyMapperList() {
        keyMapperAdapter = new KeyMapperListAdapter(activity, keyMappers, android.R.layout.simple_list_item_1,
                new String[]{"keymapper_name"}, new int[]{android.R.id.text1});
        mKeyMapperList.setAdapter(keyMapperAdapter);
        mKeyMapperList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                loadKeyMapper(i);
            }
        });
        loadKeyMappers();

    }

    private void setupKeyMapperButtons() {

        mAddKeyMapper.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                promptKeyMapperName();
            }
        });
        mRemoveKeyMapper.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                promptDeleteKeyMapper();
            }
        });
        mAddSpecialKeysButtons.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (!isKeyMapperActive())
                    return;
                advancedKey();
            }
        });
        mUseKeyMapper.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (!isKeyMapperActive())
                    return;
                useKeyMapper();
            }
        });
        mClearKey.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (!isKeyMapperActive())
                    return;
                clearKey();
            }
        });
        mRepeatKey.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (!isKeyMapperActive())
                    return;
                repeatKey();
            }
        });
        mKeyMapperName.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                if (keyMapper != null) {
                    try {
                        keyMapper.name = charSequence.toString();
                        if (selectedMap != null)
                            selectedMap.put("keymapper_name", keyMapper.name);
                        saveKeyMappers();
                        keyMapperAdapter.notifyDataSetChanged();
                    } catch (Exception ex) {
                        ex.printStackTrace();
                    }

                }
            }

            @Override
            public void afterTextChanged(Editable editable) {

            }
        });
    }

    private KeyMapper getKeyMapperObj(String keyMapper) throws IOException, ClassNotFoundException {
        byte[] bytes = Base64.decode(keyMapper, Base64.DEFAULT);
        ObjectInputStream stream = new ObjectInputStream(new ByteArrayInputStream(bytes));
        KeyMapper keyMapperObj = (KeyMapper) stream.readObject();
        stream.close();
        return keyMapperObj;
    }

    private String getKeyMapperString(KeyMapper keyMapperObj) throws IOException {
        ByteArrayOutputStream bstream = new ByteArrayOutputStream();
        ObjectOutputStream stream = new ObjectOutputStream(bstream);
        stream.writeObject(keyMapperObj);
        stream.flush();
        byte[] bytes = bstream.toByteArray();
        stream.close();
        return new String(Base64.encode(bytes, Base64.DEFAULT));
    }

    private void loadKeyMapper(int position) {
        selectedMap = keyMappers.get(position);
        KeyMapper keyMapper = (KeyMapper) selectedMap.get("key_mapper");
        if (keyMapper != null)
            loadKeyMapper(keyMapper);
    }

    private void loadKeyMapper(KeyMapper keyMapper) {
        this.keyMapper = keyMapper;
        keySurfaceView.mapping = keyMapper.mapping;
        keySurfaceView.updateDimensions();
        keySurfaceView.paint(true);
        mKeyMapperName.setText(keyMapper.name);
    }

    public void promptDeleteKeyMapper() {
        if(keyMapper == null)
            return;
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(activity.getString(R.string.KeyMapper));
        alertDialog.setMessage(activity.getString(R.string.DeleteKeyMapper));
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, activity.getString(R.string.Delete),
                new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                removeKeyMapper();
                alertDialog.dismiss();
            }
        });
        alertDialog.setButton(DialogInterface.BUTTON_NEGATIVE, activity.getString(R.string.Cancel),
                new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                alertDialog.dismiss();
            }
        });
        alertDialog.show();
    }

    private void removeKeyMapper() {
        if (keyMapper != null) {
            HashMap keyMapMap = null;
            for (HashMap<String, Object> map : keyMappers) {
                if (map.containsKey("key_mapper") && map.get("key_mapper") == keyMapper) {
                    keyMapMap = map;
                    break;
                }
            }
            if (keyMapMap != null) {
                keyMappers.remove(keyMapMap);
                saveKeyMappers();
                loadKeyMappers();
                clearKeyMapper();
            }
        }

    }

    private void clearKeyMapper() {
        keyMapper = null;
        mKeyMapperName.setText("");
        keySurfaceView.updateDimensions();
        keySurfaceView.paint(true);
    }

    private void advancedKey() {
        promptAdvancedKey();
    }

    private void promptAdvancedKey() {

        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(activity);
        alertDialogBuilder.setTitle(R.string.SpecialKeysButtons);
        final CharSequence[] items = new CharSequence[]{"Left Ctrl", "Right Ctrl", "Left Alt", "Right Alt",
                "Left Shift", "Right Shift", "Fn", "Mouse Btn Left", "Mouse Btn Middle", "Mouse Btn Right"};
        final int[] itemsKeyCodes = new int[]{KeyEvent.KEYCODE_CTRL_LEFT, KeyEvent.KEYCODE_CTRL_RIGHT,
                KeyEvent.KEYCODE_ALT_LEFT, KeyEvent.KEYCODE_ALT_RIGHT, KeyEvent.KEYCODE_SHIFT_LEFT,
                KeyEvent.KEYCODE_SHIFT_RIGHT, KeyEvent.KEYCODE_FUNCTION, Config.SDL_MOUSE_LEFT,
                Config.SDL_MOUSE_MIDDLE, Config.SDL_MOUSE_RIGHT
        };
        final boolean[] itemsEnabled = new boolean[items.length];
        alertDialogBuilder.setMultiChoiceItems(items, itemsEnabled, new DialogInterface.OnMultiChoiceClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i, boolean b) {
                int selected = 0;
                for (boolean value : itemsEnabled) {
                    if (value)
                        selected++;
                }
                if (selected == KeyMapper.KeyMapping.maxKeysButtons)
                    ToastUtils.toastShort(activity, activity.getString(R.string.TooManyKeysButtons));
                itemsEnabled[i] = b;
            }
        });
        final AlertDialog alertDialog = alertDialogBuilder.create();
        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, activity.getString(android.R.string.ok),
                new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                for (int k = 0; k < itemsEnabled.length; k++) {
                    if (itemsEnabled[k]) {
                        if (keySurfaceView != null && keySurfaceView.pointers.size() > 0) {
                            // XXX: we should only have only button pressed under edit mode
                            for (KeyMapper.KeyMapping keyMapping : keySurfaceView.pointers.values()) {
                                if (k < 7)
                                    keyMapping.addKeyCode(itemsKeyCodes[k], -1);
                                else
                                    keyMapping.addMouseButton(itemsKeyCodes[k]);
                                break;
                            }
                            saveKeyMappers();
                            keySurfaceView.paint(false);
                        }
                    }
                }
                alertDialog.dismiss();
            }
        });
        alertDialog.show();
    }

    public void promptKeyMapperName() {
        final AlertDialog alertDialog;
        alertDialog = new AlertDialog.Builder(activity).create();
        alertDialog.setTitle(activity.getString(R.string.KeyMapperName));
        final EditText keyMapperName = new EditText(activity);
        keyMapperName.setText("");
        keyMapperName.setEnabled(true);
        keyMapperName.setVisibility(View.VISIBLE);
        keyMapperName.setSingleLine();
        alertDialog.setView(keyMapperName);

        alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, activity.getString(R.string.Create),
                new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                String keyMapperNameStr = keyMapperName.getText().toString();
                for (HashMap<String, Object> keyMapperKey : keyMappers) {
                    if (keyMapperKey.containsKey(keyMapperNameStr)) {
                        ToastUtils.toastShort(activity, activity.getString(R.string.MapperExistsChooseAnotherName));
                        return;
                    }
                }
                KeyMapper nKeyMapper = new KeyMapper(keyMapperNameStr, defaultRows, defaultCols);
                HashMap<String, Object> m = new HashMap<>();
                m.put("keymapper_name", keyMapperNameStr);
                m.put("key_mapper", nKeyMapper);
                keyMappers.add(m);
                keyMapperAdapter.notifyDataSetChanged();
                loadKeyMapper(nKeyMapper);
                saveKeyMappers();
            }
        });
        alertDialog.show();
    }

    private void clearKey() {
        if (keySurfaceView != null) {
            if (keySurfaceView.pointers.size() > 0) {
                for (KeyMapper.KeyMapping keyMapping : keySurfaceView.pointers.values()) {
                    keyMapping.clear();
                    ToastUtils.toastShort(activity, activity.getString(R.string.ClearedKey));
                    break;
                }
            }
            keySurfaceView.paint(false);
            saveKeyMappers();
        }
    }

    private void repeatKey() {
        if (keySurfaceView != null) {
            for (KeyMapper.KeyMapping keyMapping : keySurfaceView.pointers.values()) {
                keyMapping.toggleRepeat();
                if (keyMapping.repeat)
                    ToastUtils.toastShort(activity, activity.getString(R.string.SetKeyRepeat));
                else
                    ToastUtils.toastShort(activity, activity.getString(R.string.RemovedKeyRepeat));
                break;
            }

            keySurfaceView.paint(false);
            saveKeyMappers();
        }
    }

    public void useKeyMapper() {
        keySurfaceView.pointers.clear();
        KeyboardUtils.hideKeyboard(activity, view);
        mapperEditLayout.setVisibility(View.GONE);
        mapperButtons.setVisibility(View.VISIBLE);
        ScreenUtils.updateOrientation(activity);
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                keySurfaceView.paint(true);
                keySurfaceView.updateDimensions();
                ToastUtils.toastShort(activity, activity.getString(R.string.UsingKeyMapper) + ": " + keyMapper.name);
            }
        }, 500);
    }

    private void loadKeyMappers() {
        keyMappers.clear();
        keyMapper = null;
        if (keySurfaceView != null)
            keySurfaceView.updateDimensions();
        Set<String> keyMappersSet = LimboSettingsManager.getKeyMappers(activity);
        if (keyMappersSet != null) {
            String[] keyMappersArr = keyMappersSet.toArray(new String[0]);

            for (String keyMapper : keyMappersArr) {
                try {
                    KeyMapper keyMapperObj = getKeyMapperObj(keyMapper);
                    HashMap<String, Object> map = new HashMap<>();
                    map.put("keymapper_name", keyMapperObj.name);
                    map.put("key_mapper", keyMapperObj);
                    keyMappers.add(map);
                } catch (IOException | ClassNotFoundException e) {
                    e.printStackTrace();
                }
            }
        }
        Collections.sort(keyMappers, new Comparator<HashMap<String, Object>>() {
            @Override
            public int compare(HashMap<String, Object> h1, HashMap<String, Object> h2) {
                return ((String) h1.get("keymapper_name")).compareTo((String) h2.get("keymapper_name"));
            }
        });
        keyMapperAdapter.notifyDataSetChanged();
    }

    public void saveKeyMappers() {
        HashSet<String> keyMappersSet = new HashSet<>();
        for (HashMap<String, Object> map : keyMappers) {
            KeyMapper mapper = (KeyMapper) map.get("key_mapper");
            if (mapper == null)
                continue;
            try {
                String keyMapperStr = getKeyMapperString(mapper);
                keyMappersSet.add(keyMapperStr);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        LimboSettingsManager.setKeyMappers(activity, keyMappersSet);
    }

    public boolean isKeyMapperActive() {
        if (keyMapper == null) {
            return false;
        } else {
            return true;
        }
    }

    public boolean isEditMode() {
        return mapperEditLayout != null && mapperEditLayout.getVisibility() == View.VISIBLE;
    }

    public void setOnSendKeyEventListener(OnSendKeyEventListener listener) {
        this.sendKeyEventListener = listener;
    }

    public void setOnSendMouseEventListener(OnSendMouseEventListener listener) {
        this.sendMouseEventListener = listener;
    }

    public void setOnUnhandledTouchEventListener(OnUnhandledTouchEventListener listener) {
        this.unhundledTouchEventListener = listener;
    }

    public interface OnSendKeyEventListener {
        void onSendKeyEvent(int keyCode, boolean down);
    }

    public interface OnSendMouseEventListener {
        void onSendMouseEvent(int keyCode, boolean down);
    }

    public interface OnUnhandledTouchEventListener {
        void OnUnhandledTouchEvent(MotionEvent event);
    }

    static class KeyMapperListAdapter extends SimpleAdapter {
        public KeyMapperListAdapter(Context context, List<? extends Map<String, ?>> data, int resource, String[] from, int[] to) {
            super(context, data, resource, from, to);
        }
    }
}
